 /*
Maria Benhammouda
simcache.cpp
*/

//necessary library
#include <bitset>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <list>
#include <math.h>
#include <queue>
#include <string>
#include <vector>
#include <fstream>
#include <iomanip>
#include <regex>
#include <cstdlib>
#include <unordered_map>

using namespace std;

// Some helpful constant values that we'll be using.
size_t const static NUM_REGS = 8; 
size_t const static MEM_SIZE = 1<<13;
size_t const static REG_SIZE = 1<<16;


/*printing functions*/
void print_cache_config(const string &cache_name, int size, int assoc, int blocksize, int num_rows) {
    cout << "Cache " << cache_name << " has size " << size <<
        ", associativity " << assoc << ", blocksize " << blocksize <<
        ", rows " << num_rows << endl;
}

void print_log_entry(const string &cache_name, const string &status, int pc, int addr, int row) {
    cout << left << setw(8) << cache_name + " " + status <<  right <<
        " pc:" << setw(5) << pc <<
        "\taddr:" << setw(5) << addr <<
        "\trow:" << setw(4) << row << endl;
}

/*cache calculations*/
int getIndex(int address, int BLOCK_SIZE, int ROWS ) {
    return (address / BLOCK_SIZE) % ROWS;
}

int getTag(int address, int BLOCK_SIZE, int ROWS) {
    return address / (BLOCK_SIZE * ROWS);
}
int getRow(int Lsize, int Lblocksize, int Lassoc) {
    return (Lsize / (Lblocksize * Lassoc));
}

/*function that takes the address, decides if hit or miss, then prints the cache output*/
bool accessCache(int address, int pc, int BLOCK_SIZE, int ROWS, int ASSOC, int CACHE_SIZE, std::unordered_map<int, int>& cache, std::list<int>& lruQueue, std::string Cache_name, uint16_t opcode) {
    int tag = getTag(address, BLOCK_SIZE, ROWS);
    int index = getIndex(address, BLOCK_SIZE, ROWS);

    // assume tag not found and initialize cache index to outside cache range
    bool found = false;
    /*insertIndex is a variable that is used to keep track of the index of the block in the cache 
    where a new tag needs to be inserted or where an existing tag is found during the cache lookup operation*/
    int insertIndex = -1;


    /*this code implements the cache lookup for a direct-mapped cache by checking if the required tag matches the current index of the cache. 
    If it is found, then the insertIndex is set to the current index. 
    If not found, then the insertIndex is still set to the current index,
     as there is only one possible location where a block can be stored in a direct-mapped cache*/
if (ASSOC == 1) {
    // direct-mapped cache
    if (cache[index] == tag) {
        found = true;
        insertIndex = index;
    } else {
        insertIndex = index;
    }
} else {
    /*The code starts by checking if the tag is already in the set-associative cache.
     If it is, the code updates the LRU queue to move the block to the back of the queue.If the tag is not in the cache, the code checks if the cache is full.
      If it is, the code evicts the LRU block in the row where the tag would be stored. 
      The code then inserts the new tag into the cache and moves it to the back of the LRU queue.If there are no empty blocks in the row,
       the code evicts the LRU block in the row and inserts the new tag into the evicted block. 
       The code then moves the new tag to the back of the LRU queue */
  
    int rowStartIndex = index * ASSOC;
    int rowEndIndex = rowStartIndex + ASSOC - 1;
    int emptyBlocks = 0;
    int lruIndex = -1;

    for (int i = rowStartIndex; i <= rowEndIndex; i++) {
        if (cache[i] == -1) {
            //if cache cell is empty, increment number of empty cells, then tag is inserteed in the empty cell
            emptyBlocks++;
            insertIndex = i;
        } else if (cache[i] == tag) {
            //if cache  has tag, then the insert tag holds index where tag is found
            found = true;
            insertIndex = i;
            lruIndex = i; //update recently used index 
            break;
        } else if (lruIndex == -1 || lruQueue.front() == i) { 
            lruIndex = i;
        } /*The code first checks if lruIndex is -1. This means that the cache is empty and there is no LRU block. In this case, the code sets lruIndex to the index of the new tag.

If lruIndex is not -1, the code checks if the front of the LRU queue is the same as the index of the new tag. 
This means that the new tag is the most recently used block.
 In this case, the code sets lruIndex to the index of the new tag.*/
    }

    if (!found && insertIndex == -1) {
        // if the cache is full and the tag is not found, evict the LRU block in the row
        int evictIndex = lruQueue.front();
        if (evictIndex >= rowStartIndex && evictIndex <= rowEndIndex) {
            // LRU block is in the same row
            cache[evictIndex] = -1;
            insertIndex = evictIndex;
        } else if (emptyBlocks > 0) {
            // LRU block is in a different row and there are empty blocks in this row
            insertIndex = rowStartIndex + emptyBlocks - 1;
        } else {
            // LRU block is in a different row and there are no empty blocks in this row
            cache[lruIndex] = -1;
            insertIndex = lruIndex;
        }
    }
}

// insert new tag in cache

cache[insertIndex] = tag;

// move the block to the back of the LRU queue for its row
lruQueue.remove(insertIndex);
lruQueue.push_back(insertIndex);

if (opcode == 5) {
    print_log_entry(Cache_name, "SW", pc, address, index);
    return false;
}
else if (found) {
    print_log_entry(Cache_name, "HIT", pc, address, index);
    return true;
}
else {
    print_log_entry(Cache_name, "MISS", pc, address, index);
    return false;
}
}



uint16_t signExtend7(bitset<7> x) {
    uint16_t result = static_cast<uint16_t>(x.to_ulong()); // Convert to uint16_t
    if (x[6] == 1) { // If the 7th bit is 1, sign extend
        result |= 0xFF80;
    }
    return result;
}

uint16_t signExtend13(bitset<13> x) {
    if (x[12] == 1) { // Check if 13th bit is 1
        x |= (0xFFFF << 13); // Set bits 13-15 to 1 (sign extension)
    }
    return static_cast<uint16_t>(x.to_ulong());
}

uint16_t zeroExtend13(bitset<13> x) {
    x &= 0x1FFF; // Bitwise AND with 0x1FFF (binary 0001111111111111)
    return static_cast<uint16_t>(x.to_ulong());
}


// responsible for executing all opcodes of 0
// imm4 holds the last 4 bits of the instruction
void executeopcode0(uint16_t &pc, uint16_t RgSrcA[], uint16_t RgSrcB[], uint16_t RgDst[], uint16_t imm4[], uint16_t (&reg)[8]) {
     if(imm4[pc&8191]==0){ //add
        if(RgDst[pc&8191]==0){
            reg[0]= 0;
        }else{
        reg[RgDst[pc&8191]]= (reg[RgSrcA[pc&8191]]+reg[RgSrcB[pc&8191]]);
         
        }
           pc=pc+1;
        }
  
     else if(imm4[pc&8191]==1){ // sub
        if(RgDst[pc&8191]==0){
            reg[0]= 0;
        }else { 
         reg[RgDst[pc&8191]]= reg[RgSrcA[pc&8191]]-reg[RgSrcB[pc&8191]];
            
        }
           pc=pc+1;
        }else if(imm4[pc&8191]==2){   // or
  
        if(RgDst[pc&8191]==0){
          reg[0]= 0;
        }else{

          reg[RgDst[pc&8191]]= (reg[RgSrcA[pc&8191]]|reg[RgSrcB[pc&8191]]);

        }
           pc=pc+1;
         }else if(imm4[pc&8191]==3){ //and

        if(RgDst[pc&8191]==0){
             reg[0]= 0;
        }else {
          reg[RgDst[pc&8191]]= (reg[RgSrcA[pc&8191]]&reg[RgSrcB[pc&8191]]);
        }
           pc=pc+1;
         }else if(imm4[pc&8191]==4){ // slt
      
        if(RgDst[pc&8191]==0){
             reg[0]= 0;
        }
        else {
           reg[RgDst[pc&8191]]= (reg[RgSrcA[pc&8191]]<reg[RgSrcB[pc&8191]]);
        }
           pc=pc+1;
        }
     else if(imm4[pc&8191]==8){ // jr
            //value of pc
          pc=reg[RgSrcA[pc&8191]];
        
        }
     else{ //if it is not any of the above, then it is a .fill, therefore we must increment coutner
         pc=pc+1;
        }
}

// addi opcode
void executeopcode1(uint16_t &pc, uint16_t RgSrcA[], uint16_t RgSrcB[], uint16_t RgDst[], bitset<7> imm7[], uint16_t (&reg)[8]) {
        if(RgSrcB[pc&8191]==0){
            reg[0]= 0;
        }else{
        
        reg[RgSrcB[pc&8191]]= (reg[RgSrcA[pc&8191]]+ signExtend7( imm7[pc&8191]));
           
        }
        
        pc=pc+1;
}

//jump and halt 
void executeopcode2(uint16_t &pc, bool &halt, const bitset<13> imm13[]) {
    if (zeroExtend13(imm13[pc & 8191]) == (pc&8191)) {
        halt = true; // if the pc is the smae as the immediate then this is a halt 
    }
    pc = zeroExtend13(imm13[pc & 8191]);
}

//jal
void executeopcode3(uint16_t &pc, uint16_t &reg7,  bitset<13> imm13[]) {
    reg7 = pc + 1; //set the next pc in reg7 
    pc = zeroExtend13(imm13[pc & 8191]);
}

//lw
void executeopcode4(uint16_t &pc, uint16_t (&reg)[8], uint16_t (&memory)[MEM_SIZE], uint16_t RgSrcA[], uint16_t RgSrcB[], bitset<7> imm7[]) {
    if(RgSrcB[pc & 8191] == 0){
        reg[0] = 0;
    }
    else {
        //only take the least significant 13 bits of the addition as the index of memory, since mem size is max 8191
        reg[RgSrcB[pc & 8191]] = memory[(signExtend7(imm7[pc & 8191]) + reg[RgSrcA[pc & 8191]]) & 8191];
    }
    pc = pc + 1;
}
// sw
void executeopcode5(uint16_t &pc, uint16_t (&reg)[8], uint16_t (&memory)[MEM_SIZE], uint16_t RgSrcA[], uint16_t RgSrcB[], bitset<7> imm7[]) {
//store the value of the register in the memory
   memory[(reg[RgSrcA[pc&8191]]+signExtend7(imm7[pc&8191]))& 8191]= reg[RgSrcB[pc&8191]];
         pc=pc+1;
}

//jeq
 void executeopcode6(uint16_t &pc, uint16_t (&reg)[8], uint16_t (&memory)[MEM_SIZE], uint16_t RgSrcA[], uint16_t RgSrcB[], bitset<7> imm7[]) {
        if(reg[RgSrcA[pc&8191]] == reg[RgSrcB[pc&8191]]){ // if the values in the registers are equal then jump to pc+imm+1 
            pc = pc+signExtend7(imm7[pc&8191])+1;
            }
        else {
          pc=pc+1;
        }
 }
 //slti
 void executeopcode7(uint16_t &pc, uint16_t (&reg)[8], uint16_t (&memory)[MEM_SIZE], uint16_t RgSrcA[], uint16_t RgSrcB[], bitset<7> imm7[]) {

        if(RgSrcB[pc&8191]==0){
           reg[0]= 0;
        }
        else {
           reg[RgSrcB[pc&8191]]= reg[RgSrcA[pc&8191]] < signExtend7(imm7[pc&8191]);
        }
           pc=pc+1;
 }
/*
    Prints the current state of the simulator, including
    the current program counter, the current register values,
    and the first memquantity elements of memory.

    @param pc The final value of the program counter
    @param regs Final value of all registers
    @param memory Final value of memory
    @param memquantity How many words of memory to dump
*/
void print_state(uint16_t pc, uint16_t regs[], uint16_t memory[], size_t memquantity) {
    cout << setfill(' ');
    cout << "Final state:" << endl;
    cout << "\tpc=" <<setw(5)<< pc << endl;

    for (size_t reg=0; reg<NUM_REGS; reg++)
        cout << "\t$" << reg << "="<<setw(5)<<regs[reg]<<endl;

    cout << setfill('0');
    bool cr = false;
    for (size_t count=0; count<memquantity; count++) {
        cout << hex << setw(4) << memory[count] << " ";
        cr = true;
        if (count % 8 == 7) {
            cout << endl;
            cr = false;
        }
    }
    if (cr)
        cout << endl;
}


/*reads data and loads it to memory cells*/
void load_data_from_file( ifstream &f, char *filename, uint16_t memory[], uint16_t &pc ,uint16_t opcode[],uint16_t imm4[],bitset<7> imm7[],  
bitset<13> imm13[],  uint16_t RgSrcA[],uint16_t RgSrcB[],uint16_t RgDst[]  ){
    if (!f.is_open()) {
        cerr << "Can't open file "<<filename<<endl;
        exit (1);
    }

    string line;    // define the standard format of the instructions 
    regex machine_code_re("^ram\\[(\\d+)\\] = 16'b(\\d+);.*$");
    size_t expectedaddr = 0;
    while (getline(f, line)) {
        smatch sm;
        if (!regex_match(line, sm, machine_code_re)) {
            cerr << "Can't parse line: " << line << endl;
            exit(1);
        }
        size_t addr = stoi(sm[1], nullptr, 10);
        unsigned instr = stoi(sm[2], nullptr, 2);
        if (addr != expectedaddr) {
            cerr << "Memory addresses encountered out of sequence: " << addr << endl;
            exit(1);
        }
        if (addr >= MEM_SIZE) {
            cerr << "Program too big for memory" << endl;
            exit(1);
        }
       //set the value of each memory cell as the value of the 16 bit instruction
        memory[addr] = instr;
        expectedaddr ++;
       }
}
/*simluate a cache of single level */
void onelevelcache(uint16_t pc, bitset<7> imm7[], uint16_t RgSrcA[],uint16_t reg[], int L1blocksize, int num_rows_l1, int L1assoc, int L1size, unordered_map<int, int>& cache1, list<int>& lruQueue1, uint16_t opcode[]){

      // Simulate cache
         bitset<13> calculatedimm= (signExtend7(imm7[pc & 8191]) + reg[RgSrcA[pc & 8191]]) & 8191;
        int address= calculatedimm.to_ulong();
       accessCache(address,  pc, L1blocksize, num_rows_l1, L1assoc, L1size, cache1, lruQueue1, "L1",  opcode[pc&8191]);
    
}
/*simluate a cache of two levels */
void twolevelcache(uint16_t pc, bitset<7> imm7[], uint16_t RgSrcA[],uint16_t reg[], int L1blocksize, int num_rows_l1, int L1assoc, int L1size, unordered_map<int, int>& cache1, list<int>& lruQueue1,int L2blocksize, int num_rows_l2, int L2assoc, int L2size, unordered_map<int, int>& cache2, list<int>& lruQueue2, uint16_t opcode[]){
  bitset<13> calculatedimm= (signExtend7(imm7[pc & 8191]) + reg[RgSrcA[pc & 8191]]) & 8191;
                 int address= calculatedimm.to_ulong();
                 bool foundincache1= accessCache(address, pc, L1blocksize, num_rows_l1, L1assoc, L1size,cache1,lruQueue1, "L1", opcode[pc&8191]);
             if(!foundincache1){
                accessCache(address, pc, L2blocksize, num_rows_l2, L2assoc, L2size,cache2,lruQueue2, "L2", opcode[pc&8191]);
            }   
} 
          

void initializecache(std::unordered_map<int, int>& cache, int Lsize, int Lassoc)
{
    for (int i = 0; i < Lsize; i++) {
        for (int j = 0; j < Lassoc; j++) {
            cache[i * Lassoc + j] = -1;
        }
    }
}

void simulatecache(vector<int> parts,string cache_config,uint16_t pc, bitset<7> imm7[], uint16_t RgSrcA[],uint16_t reg[], int L1blocksize, int num_rows_l1, int L1assoc, int L1size, unordered_map<int, int>& cache1, list<int>& lruQueue1,int L2blocksize, int num_rows_l2, int L2assoc, int L2size, unordered_map<int, int>& cache2, list<int>& lruQueue2, uint16_t opcode[])
{
         if (cache_config.size() > 0) {
        if (parts.size() == 3) {
       onelevelcache(pc,imm7,RgSrcA,reg,L1blocksize,num_rows_l1,L1assoc,L1size,cache1,lruQueue1,opcode);
        } else if (parts.size() == 6) {
            twolevelcache(pc,imm7,RgSrcA,reg,L1blocksize,num_rows_l1,L1assoc,L1size,cache1,lruQueue1,L2blocksize,num_rows_l2,L2assoc,L2size,cache2,lruQueue2,opcode);    
        } else {
            cerr << "Invalid cache config"  << endl;
            exit(-1);
        }
    }
}


int main(int argc, char *argv[]) {
     //define the necessary variables and constants 
 char *filename = nullptr;
    bool do_help = false;
    bool arg_error = false;
    string cache_config;
    unordered_map<int, int> cache1,cache2;
    list<int> lruQueue,lruQueue1,lruQueue2;      
    bool halt=false;
    bitset<13> imm13[MEM_SIZE]={0};
    uint16_t reg[NUM_REGS]={0}, RgSrcA[MEM_SIZE]={0}, RgSrcB[MEM_SIZE]={0}, RgDst[MEM_SIZE]={0},opcode[MEM_SIZE]={0},imm4[MEM_SIZE]={0},pc=0,memory[MEM_SIZE]={0};
    bitset<7>  imm7[MEM_SIZE]={0};
    vector<int> parts;
    size_t pos;
    size_t lastpos = 0;
    int L1size=0,L2size=0,L1assoc=0,L2assoc=0,L1blocksize=0,L2blocksize=0, num_rows_l1=0,num_rows_l2=0;
      
    /*get the cache configuration*/
    for (int i=1; i<argc; i++) {
        string arg(argv[i]);
        if (arg.rfind("-",0)==0) {
            if (arg== "-h" || arg == "--help")
                do_help = true;
            else if (arg=="--cache") {
                i++;
                if (i>=argc)
                    arg_error = true;
                else
                    cache_config = argv[i];
            }
            else
                arg_error = true;
        } else {
            if (filename == nullptr)
                filename = argv[i];
            else
                arg_error = true;
        }
    }

    /* Display error message if appropriate */
    if (arg_error || do_help || filename == nullptr) {
        cerr << "usage " << argv[0] << " [-h] [--cache CACHE] filename" << endl << endl; 
        cerr << "Simulate E20 cache" << endl << endl;
        cerr << "positional arguments:" << endl;
        cerr << "  filename    The file containing machine code, typically with .bin suffix" << endl<<endl;
        cerr << "optional arguments:"<<endl;
        cerr << "  -h, --help  show this help message and exit"<<endl;
        cerr << "  --cache CACHE  Cache configuration: size,associativity,blocksize (for one"<<endl;
        cerr << "                 cache) or"<<endl;
        cerr << "                 size,associativity,blocksize,size,associativity,blocksize"<<endl;
        cerr << "                 (for two caches)"<<endl;
        return 1;
    }
/*open file*/
    ifstream f(filename);
    if (!f.is_open()) {
        cerr << "Can't open file "<<filename<<endl;
        return 1;
    }
/*load the machine code from the file */
load_data_from_file(f,filename,memory,pc,opcode,imm4,imm7,imm13,RgSrcA,RgSrcB,RgDst);
    
           /*if there is a cache configuration provided, then store it in appropriate variables for size, associativity and blocksize. from there calculate the number of rows */
    if (cache_config.size() > 0) {
        while ((pos = cache_config.find(",", lastpos)) != string::npos) {
            parts.push_back(stoi(cache_config.substr(lastpos,pos)));
            lastpos = pos + 1;
        }
        parts.push_back(stoi(cache_config.substr(lastpos)));
        if (parts.size() == 3) {
            /*cache calculation*/
             L1size = parts[0];
             L1assoc = parts[1];
             L1blocksize = parts[2];
             num_rows_l1 = getRow( L1size,  L1blocksize,  L1assoc);

            print_cache_config("L1", L1size, L1assoc, L1blocksize, num_rows_l1);
            // Initialize cache1 to -1
            initializecache(cache1,L1size,L1assoc);
        
        }else if(parts.size() == 6){
            /*cache calculation*/
             L1size = parts[0];
             L1assoc = parts[1];
             L1blocksize = parts[2];
             L2size = parts[3];
             L2assoc = parts[4];
             L2blocksize = parts[5];
             num_rows_l1 = getRow( L1size,  L1blocksize,  L1assoc);
             num_rows_l2 = getRow( L2size,  L2blocksize,  L2assoc);
            
            print_cache_config("L1", L1size, L1assoc, L1blocksize, num_rows_l1);
            print_cache_config("L2", L2size, L2assoc, L2blocksize, num_rows_l2);
             // Initialize cache1 to -1
          initializecache(cache1,L1size,L1assoc);
            // Initialize cache2 to -1
         initializecache(cache2,L2size,L2assoc);

        }
    }
    /*Loop through instructions, for each memory cell calculate the opcode and assign proper registers and correct immediate values*/
    while(!halt){
      opcode[pc&8191]=((memory[pc&8191]>> 13) & 7) ;
        // define the opcode as the last 4 bits
        imm4[pc&8191]=(memory[pc&8191]& 15);
        // define the opcode as the last 13 bits
        imm13[pc&8191]=(memory[pc&8191]& 8191);
        // define the opcode as the last 7 bits
        imm7[pc&8191]=(memory[pc&8191]& 127);
        RgSrcA[pc&8191]=((memory[pc&8191]>> 10)& 7);
        RgSrcB[pc&8191]=(((memory[pc&8191]) >>7)& 7);
        RgDst[pc&8191]=(((memory[pc&8191]) >> 4)& 7); 
        
       switch (opcode[pc&8191]) {
        case 0:
        executeopcode0(pc, RgSrcA,RgSrcB,RgDst,imm4, reg);
        break;
        case 1:
        executeopcode1(pc, RgSrcA,RgSrcB,RgDst,imm7, reg);
        break;
        case 2: 
       executeopcode2(pc,halt,imm13);
        break;
        case 3:
       executeopcode3(pc, reg[7],imm13);
        break;
        case 4:
        simulatecache(parts,cache_config,pc,imm7,RgSrcA,reg,L1blocksize,num_rows_l1,L1assoc,L1size,cache1,lruQueue1,L2blocksize,num_rows_l2,L2assoc,L2size,cache2,lruQueue2,opcode);
        executeopcode4(pc,reg,memory,RgSrcA,RgSrcB,imm7);
        break;
        case 5:
        simulatecache(parts,cache_config,pc,imm7,RgSrcA,reg,L1blocksize,num_rows_l1,L1assoc,L1size,cache1,lruQueue1,L2blocksize,num_rows_l2,L2assoc,L2size,cache2,lruQueue2,opcode);
        executeopcode5(pc,reg,memory,RgSrcA,RgSrcB,imm7);
        break;
        case 6:
       executeopcode6(pc,reg,memory,RgSrcA,RgSrcB,imm7);
        break;
        case 7:
        executeopcode7(pc,reg,memory,RgSrcA,RgSrcB,imm7);
        break;
    }
   
    }
    return 0;
}
