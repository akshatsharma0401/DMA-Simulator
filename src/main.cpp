#include <iostream>
#include <vector>
#include <iomanip>
#include <algorithm>
#include <string>
#include <limits>

using namespace std;

// Represents a single contiguous partition in simulated RAM
struct MemoryBlock {
    int blockId;
    int size;           // Size in KB
    bool isFree;        // true = free hole, false = occupied
    int processId;      // -1 if free, otherwise ID of allocated process
};

// Represents an incoming process request
struct Process {
    int processId;
    int size;           // Size in KB
    bool isAllocated;
    int allocatedBlockId;
};

// Holds evaluation metrics for a single algorithm run
struct SimulationMetrics {
    string algoName;
    int allocatedCount;
    int totalProcesses;
    int allocatedMemory;
    int totalMemory;
    double utilizationPct;
    int totalFreeMemory;
    int largestFreeHole;
    int externalFragmentation;
    int blockComparisons;
};

class MemorySimulator {
private:
    vector<int> initialPartitions;
    vector<MemoryBlock> memory;
    int comparisons;

public:
    MemorySimulator(const vector<int>& partitions) : initialPartitions(partitions), comparisons(0) {
        resetMemory();
    }

    // Re-initializes memory blocks to the starting layout for fair comparison
    void resetMemory() {
        memory.clear();
        comparisons = 0;
        for (size_t i = 0; i < initialPartitions.size(); ++i) {
            memory.push_back({static_cast<int>(i + 1), initialPartitions[i], true, -1});
        }
    }

    // Splits a free block if leftover space remains after allocation
    void assignBlock(int index, Process& proc) {
        int originalBlockId = memory[index].blockId;
        int leftover = memory[index].size - proc.size;

        memory[index].size = proc.size;
        memory[index].isFree = false;
        memory[index].processId = proc.processId;

        proc.isAllocated = true;
        proc.allocatedBlockId = originalBlockId;

        if (leftover > 0) {
            MemoryBlock splitHole = {originalBlockId, leftover, true, -1};
            memory.insert(memory.begin() + index + 1, splitHole);
        }
    }

    // 1. First Fit Allocation
    bool allocateFirstFit(Process& proc) {
        for (size_t i = 0; i < memory.size(); ++i) {
            comparisons++;
            if (memory[i].isFree && memory[i].size >= proc.size) {
                assignBlock(static_cast<int>(i), proc);
                return true;
            }
        }
        return false;
    }

    // 2. Best Fit Allocation
    bool allocateBestFit(Process& proc) {
        int bestIdx = -1;
        for (size_t i = 0; i < memory.size(); ++i) {
            comparisons++;
            if (memory[i].isFree && memory[i].size >= proc.size) {
                if (bestIdx == -1 || memory[i].size < memory[bestIdx].size) {
                    bestIdx = static_cast<int>(i);
                }
            }
        }
        if (bestIdx != -1) {
            assignBlock(bestIdx, proc);
            return true;
        }
        return false;
    }

    // 3. Worst Fit Allocation
    bool allocateWorstFit(Process& proc) {
        int worstIdx = -1;
        for (size_t i = 0; i < memory.size(); ++i) {
            comparisons++;
            if (memory[i].isFree && memory[i].size >= proc.size) {
                if (worstIdx == -1 || memory[i].size > memory[worstIdx].size) {
                    worstIdx = static_cast<int>(i);
                }
            }
        }
        if (worstIdx != -1) {
            assignBlock(worstIdx, proc);
            return true;
        }
        return false;
    }

    // Frees memory held by a process and coalesces adjacent free holes
    bool deallocateProcess(int pid) {
        bool found = false;
        for (auto& block : memory) {
            if (!block.isFree && block.processId == pid) {
                block.isFree = true;
                block.processId = -1;
                found = true;
            }
        }
        if (!found) return false;

        // Adjacent hole coalescing
        for (size_t i = 0; i + 1 < memory.size(); ) {
            if (memory[i].isFree && memory[i + 1].isFree) {
                memory[i].size += memory[i + 1].size;
                memory.erase(memory.begin() + i + 1);
            } else {
                ++i;
            }
        }
        return true;
    }

    // Computes utilization, fragmentation, and overhead metrics
    SimulationMetrics calculateMetrics(const string& algoName, const vector<Process>& procs) const {
        int totalMem = 0;
        int usedMem = 0;
        int freeMem = 0;
        int largestHole = 0;

        for (const auto& block : memory) {
            totalMem += block.size;
            if (block.isFree) {
                freeMem += block.size;
                largestHole = max(largestHole, block.size);
            } else {
                usedMem += block.size;
            }
        }

        int allocatedProcs = 0;
        bool hasUnallocated = false;
        for (const auto& p : procs) {
            if (p.isAllocated) allocatedProcs++;
            else hasUnallocated = true;
        }

        double util = (totalMem > 0) ? (static_cast<double>(usedMem) / totalMem) * 100.0 : 0.0;
        int extFrag = hasUnallocated ? freeMem : 0;

        return {
            algoName,
            allocatedProcs,
            static_cast<int>(procs.size()),
            usedMem,
            totalMem,
            util,
            freeMem,
            largestHole,
            extFrag,
            comparisons
        };
    }

    // Prints visual partition layout and summary metrics
    void displayMetrics(const string& title, const vector<Process>& procs) const {
        SimulationMetrics m = calculateMetrics(title, procs);
        cout << "\n------------------------------------------------------------\n";
        cout << " ALGORITHM: " << m.algoName << "\n";
        cout << "------------------------------------------------------------\n";
        cout << " Memory Map: ";
        for (const auto& b : memory) {
            if (b.isFree) {
                cout << "[FREE: " << b.size << " KB] ";
            } else {
                cout << "[P" << b.processId << ": " << b.size << " KB] ";
            }
        }
        cout << "\n\n Process Allocation Log:\n";
        for (const auto& p : procs) {
            cout << "   Process P" << p.processId << " (" << p.size << " KB) -> ";
            if (p.isAllocated) {
                cout << "Allocated (Initial Block #" << p.allocatedBlockId << ")\n";
            } else {
                cout << "NOT ALLOCATED (Insufficient contiguous hole)\n";
            }
        }

        cout << fixed << setprecision(2);
        cout << "\n Performance Summary:\n";
        cout << "   * Processes Allocated  : " << m.allocatedCount << " / " << m.totalProcesses << "\n";
        cout << "   * Memory Utilization   : " << m.utilizationPct << "% (" << m.allocatedMemory << " / " << m.totalMemory << " KB)\n";
        cout << "   * Total Free Memory    : " << m.totalFreeMemory << " KB (Largest Hole: " << m.largestFreeHole << " KB)\n";
        cout << "   * External Fragment.   : " << m.externalFragmentation << " KB\n";
        cout << "   * Block Comparisons    : " << m.blockComparisons << "\n";
    }
};

// Safe positive integer input helper
int readPositiveInt(const string& prompt) {
    int val;
    while (true) {
        cout << prompt;
        if (cin >> val && val > 0) {
            return val;
        }
        cout << "  [Invalid input] Please enter a positive integer.\n";
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }
}

// Runs all three strategies on user-supplied blocks and processes
void runUserComparison(const vector<int>& blocks, const vector<int>& procSizes) {
    cout << "\n============================================================\n";
    cout << " LIVE SIMULATION RESULTS (USER INPUT WORKLOAD)\n";
    cout << "============================================================\n";
    cout << " Initial Partitions : [ ";
    for (int b : blocks) cout << b << " KB ";
    cout << "]\n Incoming Processes : [ ";
    for (size_t i = 0; i < procSizes.size(); ++i) cout << "P" << (i + 1) << "=" << procSizes[i] << " KB ";
    cout << "]\n";

    vector<string> names = {"First Fit", "Best Fit", "Worst Fit"};
    vector<SimulationMetrics> results;

    for (int mode = 0; mode < 3; ++mode) {
        MemorySimulator sim(blocks);
        vector<Process> procs;
        for (size_t i = 0; i < procSizes.size(); ++i) {
            procs.push_back({static_cast<int>(i + 1), procSizes[i], false, -1});
        }

        for (auto& p : procs) {
            if (mode == 0) sim.allocateFirstFit(p);
            else if (mode == 1) sim.allocateBestFit(p);
            else sim.allocateWorstFit(p);
        }

        sim.displayMetrics(names[mode], procs);
        results.push_back(sim.calculateMetrics(names[mode], procs));
    }

    // Print Side-by-Side Comparative Summary Table
    cout << "\n==================== COMPARATIVE SUMMARY ====================\n";
    cout << left << setw(26) << "Metric"
         << setw(14) << "First Fit"
         << setw(14) << "Best Fit"
         << setw(14) << "Worst Fit" << "\n";
    cout << "-------------------------------------------------------------\n";

    cout << left << setw(26) << "Processes Allocated";
    for (const auto& r : results) {
        cout << setw(14) << (to_string(r.allocatedCount) + "/" + to_string(r.totalProcesses));
    }
    cout << "\n";

    cout << left << setw(26) << "Memory Utilization (%)";
    for (const auto& r : results) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%.2f%%", r.utilizationPct);
        cout << setw(14) << buf;
    }
    cout << "\n";

    cout << left << setw(26) << "Largest Free Hole (KB)";
    for (const auto& r : results) {
        cout << setw(14) << (to_string(r.largestFreeHole) + " KB");
    }
    cout << "\n";

    cout << left << setw(26) << "External Frag. (KB)";
    for (const auto& r : results) {
        cout << setw(14) << (to_string(r.externalFragmentation) + " KB");
    }
    cout << "\n";

    cout << left << setw(26) << "Block Comparisons";
    for (const auto& r : results) {
        cout << setw(14) << r.blockComparisons;
    }
    cout << "\n=============================================================\n";
}

// Interactive session on the user's blocks for live allocate/deallocate demos
void runLiveSession(const vector<int>& blocks, const vector<int>& initialProcSizes) {
    int algoChoice;
    while (true) {
        algoChoice = readPositiveInt("\nSelect Strategy for Live Session (1 = First Fit, 2 = Best Fit, 3 = Worst Fit): ");
        if (algoChoice >= 1 && algoChoice <= 3) break;
        cout << "  [Invalid choice] Enter 1, 2, or 3.\n";
    }

    string algoName = (algoChoice == 1) ? "First Fit" : (algoChoice == 2) ? "Best Fit" : "Worst Fit";
    MemorySimulator sim(blocks);
    vector<Process> history;
    int nextPid = 1;

    // Load initial user processes into the live session
    for (int sz : initialProcSizes) {
        Process p = {nextPid++, sz, false, -1};
        if (algoChoice == 1) sim.allocateFirstFit(p);
        else if (algoChoice == 2) sim.allocateBestFit(p);
        else sim.allocateWorstFit(p);
        history.push_back(p);
    }

    sim.displayMetrics(algoName + " (Initial State)", history);

    while (true) {
        cout << "\n--- Live Memory Controls (" << algoName << ") ---\n";
        cout << " 1. Allocate a New Process\n";
        cout << " 2. Terminate/Deallocate a Process (Coalesce Free Holes)\n";
        cout << " 3. Show Current Memory Map & Metrics\n";
        cout << " 4. Back to Main Menu\n";
        int op = readPositiveInt(" Choice: ");

        if (op == 1) {
            int pSize = readPositiveInt(" Enter new process size (KB): ");
            Process p = {nextPid++, pSize, false, -1};
            bool ok = (algoChoice == 1) ? sim.allocateFirstFit(p)
                    : (algoChoice == 2) ? sim.allocateBestFit(p)
                                        : sim.allocateWorstFit(p);
            history.push_back(p);
            cout << (ok ? " -> Process P" + to_string(p.processId) + " allocated successfully.\n"
                        : " -> Allocation FAILED (External fragmentation / no block large enough).\n");
            sim.displayMetrics(algoName, history);
        } else if (op == 2) {
            int pid = readPositiveInt(" Enter Process ID to terminate (e.g., 1 for P1): ");
            if (sim.deallocateProcess(pid)) {
                for (auto& h : history) {
                    if (h.processId == pid) h.isAllocated = false;
                }
                cout << " -> Process P" << pid << " terminated and adjacent free holes merged.\n";
                sim.displayMetrics(algoName, history);
            } else {
                cout << " -> Process P" << pid << " is not currently active in RAM.\n";
            }
        } else if (op == 3) {
            sim.displayMetrics(algoName, history);
        } else if (op == 4) {
            break;
        } else {
            cout << " Invalid option.\n";
        }
    }
}

int main() {
    cout << "============================================================\n";
    cout << " CSE234 - DYNAMIC MEMORY ALLOCATION SIMULATOR (PROJECT #8)\n";
    cout << "============================================================\n";

    vector<int> blocks;
    vector<int> procSizes;

    auto inputWorkload = [&]() {
        blocks.clear();
        procSizes.clear();

        int numBlocks = readPositiveInt("\nEnter number of memory blocks: ");
        for (int i = 0; i < numBlocks; ++i) {
            blocks.push_back(readPositiveInt("  Enter size of Block #" + to_string(i + 1) + " (KB): "));
        }

        int numProcs = readPositiveInt("\nEnter number of processes: ");
        for (int i = 0; i < numProcs; ++i) {
            procSizes.push_back(readPositiveInt("  Enter size of Process P" + to_string(i + 1) + " (KB): "));
        }
    };

    // Immediately collect user input at startup and run comparison
    inputWorkload();
    runUserComparison(blocks, procSizes);

    while (true) {
        cout << "\n============================================================\n";
        cout << " NEXT ACTIONS\n";
        cout << "============================================================\n";
        cout << " 1. Re-run Side-by-Side Comparison (Current Input)\n";
        cout << " 2. Live Interactive Allocate / Deallocate Session\n";
        cout << " 3. Enter New Memory Blocks & Processes\n";
        cout << " 4. Exit\n";
        int choice = readPositiveInt(" Select an option: ");

        if (choice == 1) {
            runUserComparison(blocks, procSizes);
        } else if (choice == 2) {
            runLiveSession(blocks, procSizes);
        } else if (choice == 3) {
            inputWorkload();
            runUserComparison(blocks, procSizes);
        } else if (choice == 4) {
            cout << "Exiting simulator.\n";
            break;
        } else {
            cout << "Invalid option. Try again.\n";
        }
    }

    return 0;
}