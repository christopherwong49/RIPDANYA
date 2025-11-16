#pragma once

#include "includes.hpp"
#include "move.hpp"

#define TTsize 2097152

enum TTflag{
    upper,
    lower,
    exact
};

struct Entry{
    uint64_t ttkey;
    Move move;
    uint16_t depth;
    Value score;
    TTflag flag;
}

class TT{
    public:
        void clear();
        Entry *probe(uint64 key);
        void store(uint64_t key, Move move, int depth, Value score, TTflag flag);
    private:
        std::vector<Entry> table;
        
}