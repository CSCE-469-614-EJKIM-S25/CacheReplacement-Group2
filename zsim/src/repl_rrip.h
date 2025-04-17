#ifndef RRIP_REPL_H_
#define RRIP_REPL_H_

#include "repl_policies.h"

// Static RRIP
class SRRIPReplPolicy : public ReplPolicy
{
protected:
    // add class member variables here
    int8_t maxRPV;
    int8_t *array;
    uint32_t numLines;

public:
    // add member methods here, refer to repl_policies.h
    explicit SRRIPReplPolicy(int8_t _maxRPV, uint32_t _numLines) : maxRPV(_maxRPV), numLines(_numLines)
    {
        array = gm_calloc<int8_t>(numLines);

        // initialize all invalid entries to max rpv
        for (uint32_t i = 0; i < numLines; ++i)
        {
            array[i] = maxRPV;
        }
    }

    ~SRRIPReplPolicy()
    {
        gm_free(array);
    }

    void increment_all()
    { // increment the RRPV for all the array items by 1
        for (uint32_t i = 0; i < numLines; ++i)
        {
            if (array[i] < maxRPV)
                ++array[i];
        }
    }

    void update(uint32_t id, const MemReq *req)
    { // sets the RRPV value to zero on cache hit
        // replaced() function sets the RRPV value to -1 so that we can tell
        // when we are updating after a cache hit or cache miss
        if (array[id] == -1) // if recently replaced, then set RRPV to 2^M - 2 (M = RRPV bits)
            array[id] = 2;
        else // if it is a hit, then we set RRPV value to zero
            array[id] = 0;
    }

    void replaced(uint32_t id)
    {
        array[id] = -1;
    }

    template <typename C>
    inline uint32_t rank(const MemReq *req, C cands)
    {
        uint32_t bestCand = -1;
        int8_t bestScore = 0;
        for (auto ci = cands.begin(); ci != cands.end(); ci.inc())
        {
            int8_t s = score(*ci);

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
    inline int8_t score(uint32_t id)
    {
        return (array[id]);
    }
};
#endif // RRIP_REPL_H_