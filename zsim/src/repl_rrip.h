#ifndef RRIP_REPL_H_
#define RRIP_REPL_H_

#include "repl_policies.h"

// Static RRIP
/*
Want to attach a RRPV value to each candidate. They are initialized
to 3 and subsequent transformations follow the below steps:


Cache hit:
    - Set RRPV of block to 0

Cache Miss:
    1. Search for first 3 from left
    2. If 3 found, go to step 5     
    3. Increment all RRPV's
    4. Go to step 1)
    5. Replace block and set RRPV to 2    
*/



class SRRIPReplPolicy : public ReplPolicy {
    protected:
        // add class member variables here
        uint32_t numLines;
        uint32_t rpvmax;
        uint32_t* rrpv_array;
        bool* recently_inserted;


    public:
        // add member methods here, refer to repl_policies.h
        // constructor
        explicit SRRIPReplPolicy(uint32_t _numLines, uint32_t _rpvmax) : numLines(_numLines), rpvmax(_rpvmax) {
            // this array holds the RRPV values for each cache line
            rrpv_array = gm_calloc<uint32_t>(numLines);
            recently_inserted = gm_calloc<bool>(numLines); // Allocate space for insertion flags

            // initialize each element to 3 as per the SRRIP policy.
            for (uint32_t i = 0; i < numLines; i++) {
                rrpv_array[i] = 3;
                recently_inserted[i] = false;
            }
        }
        // desctructor
        ~SRRIPReplPolicy() {
            gm_free(rrpv_array);
        }

        void update(uint32_t id, const MemReq* req) {
            // The update() method is called on a hit. On a hit
            // we must set the RRPV value to 0. This function is also
            // called on a miss and we don't want to set it to zero when this
            // occurs. Thus, check if RRPV is 3 first
            if (!recently_inserted[id]) {
                rrpv_array[id] = 0;
            }
            recently_inserted[id] = false;
        }

        void replaced(uint32_t id) {
            // replaced is only called on a miss. Set to 2
            rrpv_array[id] = 2;
            recently_inserted[id] = true;
        }

        template <typename C> inline uint32_t rank(const MemReq* req, C cands) {
            // loop through and determine candidate to evict
            while (true) {
                // find first element with RRPV of 3
                for (auto ci = cands.begin(); ci != cands.end(); ci.inc()) {
                    // replace block 
                    if (rrpv_array[*ci] == 3) {
                        return *ci;
                    }
                }

                // if no element w/ RRPV = 3, increment all RRPV's
                for (auto ci = cands.begin(); ci != cands.end(); ci.inc()) {
                    // cannot increment above 3
                    if (rrpv_array[*ci] < 3) {
                        // increment
                        rrpv_array[*ci]++;
                    }
                }

                // go back to finding first RRPV=3 from left.
            }
        }
        
        DECL_RANK_BINDINGS;
    
};
#endif // RRIP_REPL_H_