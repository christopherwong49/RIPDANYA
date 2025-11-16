#include "ttable.hpp"


void TT::clear(){
    std::memset(table.data(), 0, table.size() * sizeof(Entry));

}



Entry* TT::probe(uint64_t key){
    Entry &entry = table[key%TTsize];
    if(entry.key == key){
        return &entry;
    }
    return nullptr;
}

void TT::store(uint64_t key, Move move, int depth, Value score, Flag flag){
    Entry &entry = table[key%TTsize];
    entry.key = key;
    entry.move = move;
    entry.depth = depth;
    entry.score = score;
    entry.flag = flag;
    
}