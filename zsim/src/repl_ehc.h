#ifndef EHC_REPL_H_
#define EHC_REPL_H_

#include "repl_policies.h"

// EHC - An extension of SRRIP
class EHCReplPolicy : public ReplPolicy
{
protected:
  // add class member variables here
  int8_t maxRPV;

  typedef struct
  {
    int8_t RPV;
    uint64_t lineAddr;
  } RPVMeta;
  RPVMeta *array; // array for storing RPV values

  uint32_t numLines; // number of lines in the cache

  uint8_t currentHitCounter;
  uint8_t expectedFurtherHits; // each cache line has CHCounter and expected further hits
  uint8_t *CHC_array;
  uint8_t *EFH_array;

  typedef struct
  {
    bool valid;
    uint32_t tag;             // top 20 bits of block address
    uint8_t lruRecency;       // 4 bit recency counter
    uint8_t hitCountQueue[4]; // four 3 bit counters
  } HHTEntry;

  HHTEntry **HHT;

public:
  // add member methods here, refer to repl_policies.h
  explicit EHCReplPolicy(int8_t _maxRPV, uint32_t _numLines) : maxRPV(_maxRPV), numLines(_numLines)
  {
    array = gm_calloc<RPVMeta>(numLines);
    HHT = (HHTEntry **)calloc(128, sizeof(HHTEntry *));
    for (int i = 0; i < 128; i++)
    {
      HHT[i] = (HHTEntry *)calloc(16, sizeof(HHTEntry));
    }
    CHC_array = gm_calloc<uint8_t>(numLines);
    EFH_array = gm_calloc<uint8_t>(numLines);

    // initialize all invalid entries to max rpv
    for (uint32_t i = 0; i < numLines; ++i)
    {
      array[i].RPV = maxRPV;
      CHC_array[i] = 0;
      EFH_array[i] = 1; // NEED CONFIRMATION ON INITIALIZATION
    }
  }

  ~EHCReplPolicy()
  {
    gm_free(array);
    for (int i = 0; i < 128; i++)
    {
      if (HHT[i] != NULL)
      {
        free(HHT[i]);
      }
    }
    free(HHT);
  }

  void increment_all()
  { // increment the RRPV for all the array items by 1
    for (uint32_t i = 0; i < numLines; ++i)
    {
      if (array[i].RPV < maxRPV)
        ++array[i].RPV;
    }
  }

  void update(uint32_t id, const MemReq *req)
  {                                                 // sets the RRPV value to zero on cache hit
    uint64_t tag = (req->lineAddr >> 13) & 0xFFFFF; // first 20 bits are the tag
    uint32_t index = (req->lineAddr >> 6) & 0x7F;   // next 7 bits for HHT set index, assuming 128 sets like th
    // replaced() function sets the RRPV value to -1 so that we can tell
    // when we are updating after a cache hit or cache miss
    if (array[id].RPV == -1)
    { // MISS
      // if recently replaced, then set RRPV to 2^M - 2 (M = RRPV bits)
      array[id].RPV = 2;

      // change the array's lineAddr to the new lineAddr
      array[id].lineAddr = req->lineAddr;

      // Set CHC to 0
      CHC_array[id] = 0;

      // The HHT has already been updated if a cache block has been evicted
      //  Now need to calculate EHR of new cache block
      HHTEntry *entry = searchHHT(tag, index);

      if (entry == NULL)
      {
        // if the new cache block isn't in the HHT, set the EHR to 1
        EFH_array[id] = 1;
      }
      else
      {
        // if the new cache block is already in the HHT, set the EHR to the average of the HCQ
        EFH_array[id] = (entry->hitCountQueue[0] + entry->hitCountQueue[1] + entry->hitCountQueue[2] + entry->hitCountQueue[3] + 2) / 4;
      }
    }
    else
    { // HIT
      // RRPV to 0
      array[id].RPV = 0;

      // Current Hit Counter up, if it maxes out, then make sure to update the HHT
      CHC_array[id]++;
      if (CHC_array[id] == 7)
      {
        updateHHT(id, req->lineAddr);
        CHC_array[id] = 0;
      }
      // Expected Further Hits down
      // if (EFH_array[id] > 0)
        EFH_array[id]--;
    }
  }

  void updateHHT(uint32_t id, uint64_t lineAddress)
  {
    uint64_t tag = (lineAddress >> 13) & 0xFFFFF; // first 20 bits are the tag
    uint32_t index = (lineAddress >> 6) & 0x7F;   // next 7 bits for HHT set index, assuming 128 sets like th
    HHTEntry *entry = searchHHT(tag, index);
    if (entry == NULL)
    { // HHT miss
      // add it to the table
      addHHTEntry(tag, index, CHC_array[id]);
    }
    else
    { // HHT hit
      for (int i = 3; i > 0; i--)
      {
        // Shift existing hit counts to make room at the front for new one
        entry->hitCountQueue[i] = entry->hitCountQueue[i - 1];
      }
      // Push new current hit counter to front
      entry->hitCountQueue[0] = CHC_array[id];
    }
  }

  HHTEntry *searchHHT(uint32_t tag, uint32_t index)
  {
    // We know the index (vertical) from 7 bits of addy, now we search each of the ways for a match
    for (int way = 0; way < 16; way++)
    {
      HHTEntry *entry = &HHT[index][way];
      if (entry->valid && entry->tag == tag)
      {
        return &HHT[index][way]; // hit
      }
    }
    // miss
    return NULL;
  }

  void addHHTEntry(uint64_t tag, uint32_t index, uint8_t currentHitCount)
  {
    // ADDS NEW ENTRY TO HHT TABLE, EVICTS LRU ENTRY
    int victimWay = -1;
    uint8_t maxLRU = 0;

    for (int way = 0; way < 16; way++)
    {
      if (!HHT[index][way].valid)
      {
        // empty slot available immediately
        victimWay = way;
        break;
      }
      if (HHT[index][way].lruRecency > maxLRU)
      {
        maxLRU = HHT[index][way].lruRecency;
        victimWay = way;
      }
    }

    // Overwrite victim
    HHTEntry *victim = &HHT[index][victimWay];
    victim->valid = true;
    victim->tag = tag;
    victim->lruRecency = 0;

    // Clear old hit history
    for (int i = 0; i < 4; i++)
    {
      victim->hitCountQueue[i] = 0;
    }

    // Insert current hit count at head of hit history queue
    victim->hitCountQueue[0] = currentHitCount;

    // After inserting, increment LRU recency of other entries in set
    for (int way = 0; way < 16; way++)
    {
      if (way != victimWay && HHT[index][way].valid)
      {
        HHT[index][way].lruRecency++;
      }
    }
  }

  void replaced(uint32_t id)
  {
    // Flag the block as replaced for SRRIP and EHC
    array[id].RPV = -1;

    // Now we record the CHC and push it to the front of the queue
    updateHHT(id, array[id].lineAddr);
  }

  template <typename C>
  inline uint32_t rank(const MemReq *req, C cands)
  {
    uint32_t bestCand = -1;
    int32_t bestScore = INT32_MAX;
    while (true)
    {
      for (auto ci = cands.begin(); ci != cands.end(); ci.inc())
      {
        int32_t s = score(*ci);

        if (s <= bestScore)
        {
          bestCand = *ci;
          bestScore = s;
        }

        if (bestScore == -3)
          // take first -3  (minimum because EHR=0, RRPV=3)
          return bestCand;
      }
      // if all rrpv values are 3 (maxed out) and we still don't have a candidate with -3 score
      // return the most recent best candidate
      bool all_3 = true;
      for (uint32_t i = 0; i < numLines; ++i)
      {
        if (array[i].RPV != 3)
        {
          all_3 = false;
          break;
        }
      }
      if (all_3)
        return bestCand;

      // increments all RRPV's by 1
      for (uint32_t i = 0; i < numLines; ++i)
      {
        if (array[i].RPV < 3)
        {
          ++array[i].RPV;
        }
      }
    }

    // if bestScore > -3, increment all the RPV values
    // this->increment_all();
    // return rank(req, cands);
  }

  DECL_RANK_BINDINGS;

private:
  inline int32_t score(uint32_t id)
  {
    // EHR - RRPV
    return (int32_t)(EFH_array[id] - array[id].RPV);
  }
};
#endif // EHC_REPL_H_
