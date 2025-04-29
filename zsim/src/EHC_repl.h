#ifndef RRIP_REPL_H_
#define RRIP_REPL_H_

#include "repl_policies.h"

class EHCReplPolicy : public ReplPolicy {
    protected:
        struct HHT_Entry {
            bool isValid;
            uint32_t LRU_recency;
            uint64_t tag;
            uint8_t HCQ[4];
        };
    
        uint32_t* rrpv_array;         // RRPV values for each line
        bool* recently_inserted;      // Track recently inserted lines
        HHT_Entry* hht_array;         // Hit History Table
        uint8_t* expected_further_hits; // EFH for each line
        uint8_t* curr_hit_counter;    // Current hit counter for each line
        uint64_t* lineAddr_array;     // Store line addresses to update HHT on eviction
    
        uint32_t numLines;
        uint32_t rpvmax;
        int hht_num_sets;
        int hht_num_ways;
    
    public:
        explicit EHCReplPolicy(uint32_t _numLines, uint32_t _rpvmax)
            : numLines(_numLines), rpvmax(_rpvmax) {
            // Initialize arrays
            rrpv_array = gm_calloc<uint32_t>(numLines);
            recently_inserted = gm_calloc<bool>(numLines);
            hht_num_sets = 128;
            hht_num_ways = 16;
            hht_array = gm_calloc<HHT_Entry>(hht_num_sets * hht_num_ways);
            curr_hit_counter = gm_calloc<uint8_t>(numLines);
            expected_further_hits = gm_calloc<uint8_t>(numLines);
            lineAddr_array = gm_calloc<uint64_t>(numLines);
    
            // Initialize HHT entries
            for (uint32_t i = 0; i < static_cast<uint32_t>(hht_num_sets * hht_num_ways); ++i) {
                hht_array[i].isValid = false;
                hht_array[i].LRU_recency = 0;
                hht_array[i].tag = 0;
                for (int j = 0; j < 4; ++j) {
                    hht_array[i].HCQ[j] = 0;
                }
            }
    
            // Initialize RRPV to distant reuse
            for (uint32_t i = 0; i < numLines; ++i) {
                rrpv_array[i] = rpvmax - 1;
                curr_hit_counter[i] = 0;
                expected_further_hits[i] = 1; // Default EFH is 1
                recently_inserted[i] = false;
            }
        }
    
        ~EHCReplPolicy() {
            gm_free(rrpv_array);
            gm_free(recently_inserted);
            gm_free(hht_array);
            gm_free(expected_further_hits);
            gm_free(curr_hit_counter);
            gm_free(lineAddr_array);
        }
    
        void update_hht(uint32_t id, uint64_t lineAddr) {
            uint32_t setIndex = lineAddr & 0x7F; // Use 7 bits for set index (128 sets)
            uint64_t tag = lineAddr >> 7;
            uint32_t base = setIndex * hht_num_ways;
            
            // Try to find matching entry
            int wayHit = -1;
            for (int way = 0; way < hht_num_ways; ++way) {
                uint32_t idx = base + way;
                if (hht_array[idx].isValid && hht_array[idx].tag == tag) {
                    wayHit = way;
                    break;
                }
            }
            
            uint32_t insertIdx;
            if (wayHit == -1) {
                // No match: find LRU way to replace
                uint32_t maxRecency = 0;
                int insertWay = 0;
                for (int way = 0; way < hht_num_ways; ++way) {
                    uint32_t idx = base + way;
                    if (hht_array[idx].LRU_recency > maxRecency) {
                        insertWay = way;
                        maxRecency = hht_array[idx].LRU_recency;
                    }
                }
                
                insertIdx = base + insertWay;
                hht_array[insertIdx].isValid = true;
                hht_array[insertIdx].tag = tag;
                
                // Initialize HCQ for new entry
                for (int j = 0; j < 4; ++j) {
                    hht_array[insertIdx].HCQ[j] = 0;
                }
                
                // Make new entry MRU and age others
                for (int way = 0; way < hht_num_ways; ++way) {
                    uint32_t idx = base + way;
                    if (idx != insertIdx && hht_array[idx].isValid) {
                        hht_array[idx].LRU_recency++;
                    }
                }
                hht_array[insertIdx].LRU_recency = 0;
            } else {
                // Found match: update LRU
                insertIdx = base + wayHit;
                uint32_t oldRecency = hht_array[insertIdx].LRU_recency;
                
                // Make entry MRU and adjust others
                for (int way = 0; way < hht_num_ways; ++way) {
                    uint32_t idx = base + way;
                    if (idx != insertIdx && hht_array[idx].isValid && hht_array[idx].LRU_recency < oldRecency) {
                        hht_array[idx].LRU_recency++;
                    }
                }
                hht_array[insertIdx].LRU_recency = 0;
            }
            
            // Update HCQ with hit counter value (saturate at 7)
            for (int i = 3; i > 0; --i) {
                hht_array[insertIdx].HCQ[i] = hht_array[insertIdx].HCQ[i-1];
            }
            hht_array[insertIdx].HCQ[0] = std::min(curr_hit_counter[id], (uint8_t)7);
            
            // Reset hit counter
            curr_hit_counter[id] = 0;
        }
    
        uint8_t calculate_efh(uint64_t lineAddr) {
            uint32_t setIndex = lineAddr & 0x7F;
            uint64_t tag = lineAddr >> 7;
            uint32_t base = setIndex * hht_num_ways;
            
            // Try to find matching entry
            for (int way = 0; way < hht_num_ways; ++way) {
                uint32_t idx = base + way;
                if (hht_array[idx].isValid && hht_array[idx].tag == tag) {
                    // Calculate average of HCQ
                    uint32_t sum = 0;
                    for (int i = 0; i < 4; ++i) {
                        sum += hht_array[idx].HCQ[i];
                    }
                    return (sum + 2) / 4; // Round to nearest integer
                }
            }
            
            // Default EFH value if entry not found
            return 1;
        }
    
        void update(uint32_t id, const MemReq* req) override {
            // Store line address for future reference
            lineAddr_array[id] = req->lineAddr;
            
            if (!recently_inserted[id]) {
                // Reset RRPV on hit (indicating frequent reuse)
                rrpv_array[id] = 0;
                
                // Increment hit counter
                if (curr_hit_counter[id] < 255) {
                    curr_hit_counter[id]++;
                } else {
                    // If counter saturates, update HHT and reset
                    update_hht(id, req->lineAddr);
                }
                
                // Decrement EFH if it's greater than zero
                if (expected_further_hits[id] > 0) {
                    expected_further_hits[id]--;
                }
            }
            
            // Clear recently inserted flag
            recently_inserted[id] = false;
        }
    
        void replaced(uint32_t id) override {
            // Update HHT with current hit count before eviction
            update_hht(id, lineAddr_array[id]);
            
            // Calculate EFH for new line
            uint64_t newLineAddr = lineAddr_array[id];  // This will be the address of the new line
            expected_further_hits[id] = calculate_efh(newLineAddr);
            
            // Set RRPV for new line (intermediate value)
            rrpv_array[id] = rpvmax - 1;
            
            // Mark as recently inserted
            recently_inserted[id] = true;
            
            // Reset hit counter for new line
            curr_hit_counter[id] = 0;
        }
    
        template <typename C>
        uint32_t rank(const MemReq* req, C cands) {
            // Initialize best candidate with first candidate
            uint32_t best_id = *cands.begin();
            int32_t best_score = (int32_t)expected_further_hits[best_id] - (int32_t)rrpv_array[best_id];
    
            // Find candidate with minimum EFH-RRPV score
            for (auto ci = cands.begin(); ci != cands.end(); ci.inc()) {
                uint32_t id = *ci;
                int32_t score = (int32_t)expected_further_hits[id] - (int32_t)rrpv_array[id];
                if (score < best_score) {
                    best_id = id;
                    best_score = score;
                }
            }
            
            return best_id;
        }
    
        DECL_RANK_BINDINGS;
};
#endif // RRIP_REPL_H_