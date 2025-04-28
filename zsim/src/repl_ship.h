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
    explicit SHIPReplPolicy(uint64_t _maxRPV, uint32_t _numLines, uint64_t _SHCT_size) 
        : maxRPV(_maxRPV),
          numLines(_numLines),
          SHCT_size(_SHCT_size),
          evicted_cacheID(UINT64_MAX),
          addr_size(64)
    {
        SHCT = gm_calloc<uint64_t>(SHCT_size);
        block_meta = gm_calloc<BlockMeta>(numLines);

        signature_size = logBase2(SHCT_size);

        for (uint64_t i = 0; i < SHCT_size; ++i)
        {
            SHCT[i] = maxRPV;
        }

        for (uint32_t i = 0; i < numLines; ++i)
        {
            block_meta[i].signature = 0;
            block_meta[i].outcome_bit = false;
        }
    }

    ~SHIPReplPolicy()
    {
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
        uint64_t signature_m = lineAddr >> (addr_size - signature_size);
        uint64_t mask = (1ULL << signature_size) - 1;

        return signature_m & mask;
    }

    void increment_all()
    { // increment all SHCT values by 1
        for (uint64_t i = 0; i < SHCT_size; ++i)
        {
            if (SHCT[i] < maxRPV)
                ++SHCT[i];
        }
    }

    void update(uint32_t id, const MemReq *req)
    {
        BlockMeta &meta = block_meta[id];

        if (evicted_cacheID == static_cast<uint64_t>(id))
        { // eviction

            // could probably move this if statement to replaced function
            if (meta.outcome_bit != true)
            {
                if (SHCT[meta.signature] > 0)
                    --SHCT[meta.signature];
            }

            meta.outcome_bit = false;
            meta.signature = create_signature(req->lineAddr);

            if (SHCT[meta.signature] == 0)
                SHCT[meta.signature] = maxRPV; // predict distance re-reference
            else
                SHCT[meta.signature] = maxRPV >> 1; // predict intermediate re-reference
        }
        else
        { // hit
            meta.outcome_bit = true;

            if (SHCT[meta.signature] < maxRPV)
                ++SHCT[meta.signature];
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
        for (auto ci = cands.begin(); ci != cands.end(); ci.inc())
        {
            uint64_t s = score(*ci);

            if (s >= bestScore)
            {
                bestCand = *ci;
                bestScore = s;
            }

            if (bestScore == this->maxRPV)
                return bestCand;
        }

        // if bestScore < maxRPV, increment all the RPV values
        this->increment_all();
        return rank(req, cands);
    }

    DECL_RANK_BINDINGS;

private:
    inline uint64_t score(uint32_t id)
    {
        BlockMeta meta = block_meta[id];

        return (SHCT[meta.signature]);
    }
};

#endif // REPL_SHIP_H_