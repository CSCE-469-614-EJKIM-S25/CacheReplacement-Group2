#ifndef REPL_SHIP_H_
#define REPL_SHIP_H_

#include "repl_policies.h"

// SHIP
class SHIPReplPolicy : public ReplPolicy
{
protected:
    // class member variables
    uint64_t maxRPV;
    uint32_t numLines;
    uint64_t *array;

    uint64_t maxSHCTVal;
    uint64_t *SHCT;
    uint64_t SHCT_size;
    uint64_t signature_size;

    uint64_t evicted_cacheID;

    uint64_t addr_size;

    struct BlockMeta
    {
        uint64_t signature;
        bool outcome_bit;
    };
    BlockMeta *block_meta;



public:
    explicit SHIPReplPolicy(uint64_t _maxRPV, uint32_t _numLines, uint64_t _maxSHCTVal, uint64_t _SHCT_size) 
        : maxRPV(_maxRPV),
          numLines(_numLines),
          maxSHCTVal(_maxSHCTVal),
          SHCT_size(_SHCT_size),
          evicted_cacheID(UINT64_MAX),
          addr_size(64)
    {
        array = gm_calloc<uint64_t>(numLines);
        SHCT = gm_calloc<uint64_t>(SHCT_size);
        block_meta = gm_calloc<BlockMeta>(numLines);

        signature_size = logBase2(SHCT_size);

        for (uint64_t i = 0; i < SHCT_size; ++i)
        {
            SHCT[i] = maxSHCTVal >> 1;
        }

        for (uint32_t i = 0; i < numLines; ++i)
        {
            array[i] = maxRPV;

            block_meta[i].signature = 0;
            block_meta[i].outcome_bit = false;
        }
    }

    ~SHIPReplPolicy()
    {
        gm_free(array);
        gm_free(SHCT);
        gm_free(block_meta);
    }

    uint64_t logBase2(uint64_t n)
    {
        uint64_t log_val = 0;
        while (n >>= 1)
            log_val++;
        
        return log_val;
    }

    uint64_t create_signature(uint64_t lineAddr)
    {
        // Mix the bits using multiplication by a large prime number
        uint64_t x = lineAddr;
        x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
        x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
        x = x ^ (x >> 31);
        
        // Apply mask to get the desired signature size
        uint64_t mask = (1ULL << signature_size) - 1;
        return x & mask;
    }

    void update(uint32_t id, const MemReq *req)
    {
        BlockMeta &meta = block_meta[id];

        if (evicted_cacheID == static_cast<uint64_t>(id))
        { // eviction

            if (meta.outcome_bit != true)
            {
                if (SHCT[meta.signature] > 0)
                    --SHCT[meta.signature];
            }

            meta.outcome_bit = false;
            meta.signature = create_signature(req->lineAddr);

            if (SHCT[meta.signature] == 0)
                array[id] = maxRPV; // predict distance re-reference
            else
                array[id] = maxRPV - 1; // predict intermediate re-reference
        }
        else
        { // hit
            meta.outcome_bit = true;

            if (SHCT[meta.signature] < maxSHCTVal)
                ++SHCT[meta.signature];

            array[id] = 0;
        }

        evicted_cacheID = UINT64_MAX;
    }

    void replaced(uint32_t id)
    {
        evicted_cacheID = static_cast<uint64_t>(id);
    }


    template <typename C>
    inline uint32_t rank(const MemReq *req, C cands)
    {
        uint32_t bestCand = -1;
        uint64_t bestScore = 0;

        while (true)
        {
            for (auto ci = cands.begin(); ci != cands.end(); ci.inc())
            {
                if (array[*ci] >= bestScore)
                {
                    bestCand = *ci;
                    bestScore = array[*ci];
                }

                if (bestScore == this->maxRPV)
                    return bestCand;
            }

            // if no element with rrpv of 3, increment all rrpv's
            for (auto ci = cands.begin(); ci != cands.end(); ci.inc())
            {
                if (array[*ci] < 3)
                    ++array[*ci];
            }

        }
        
        // if bestScore < maxRPV, increment all the RPV values
    }

    DECL_RANK_BINDINGS;
};

#endif // REPL_SHIP_H_