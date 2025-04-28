#ifndef REPL_SHIP_H_
#define REPL_SHIP_H_

#include "repl_policies.h"
#include <unordered_map>

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

    uint64_t *id_to_lineAddr;

    uint64_t evicted_cacheID;

    uint64_t addr_size;

    // unordered_map of outcome bits, indexed by addr line
    std::unordered_map<uint64_t, bool> outcome_bits;


public:
    explicit SHIPReplPolicy(uint64_t _maxRPV, uint32_t _numLines, uint64_t _SHCT_size) 
        : maxRPV(_maxRPV),
          numLines(_numLines),
          SHCT_size(_SHCT_size),
          evicted_cacheID(UINT64_MAX),
          addr_size(64)
    {
        SHCT = gm_calloc<uint64_t>(SHCT_size);
        id_to_lineAddr = gm_calloc<uint64_t>(numLines);

        signature_size = logBase2(SHCT_size);

        for (uint64_t i = 0; i < SHCT_size; ++i)
        {
            SHCT[i] = maxRPV;
        }

        for (uint64_t i = 0; i < numLines; ++i)
        {
            id_to_lineAddr[i] = UINT64_MAX;
        }
    }

    ~SHIPReplPolicy()
    {
        gm_free(SHCT);
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
        uint64_t lineAddr = req->lineAddr;
        uint64_t signature_m = create_signature(lineAddr);

        if (evicted_cacheID == static_cast<uint64_t>(id))
        { // eviction
            uint64_t evicted_lineAddr = id_to_lineAddr[id];

            if (outcome_bits.find(evicted_lineAddr) != outcome_bits.end() && outcome_bits[evicted_lineAddr] != true && evicted_lineAddr != UINT64_MAX) 
            {
                if (SHCT[create_signature(evicted_lineAddr)] > 0)
                    --SHCT[create_signature(evicted_lineAddr)];
            }

            if (SHCT[signature_m] == 0)
                SHCT[signature_m] = maxRPV;
            else
                SHCT[signature_m] = maxRPV >> 1;
        }
        else
        { // hit
            outcome_bits[lineAddr] = true;

            if (SHCT[signature_m] < maxRPV)
                ++SHCT[signature_m];
        }

        id_to_lineAddr[id] = lineAddr;
        evicted_cacheID = -1;
    }

    void replaced(uint32_t id)
    {
        evicted_cacheID = static_cast<int>(id);
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
        uint64_t cand_lineAddr = id_to_lineAddr[id];
        uint64_t cand_signature = create_signature(cand_lineAddr);

        return (SHCT[cand_signature]);
    }
};

#endif // REPL_SHIP_H_