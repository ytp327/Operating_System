#include <iostream>
#include <stdlib.h>  /* atoi */
#include <unistd.h>  /* getopt */
#include <string.h>  /* strcmp */
#include <math.h>    /* pow */
#include <fstream>
#include <istream>  
#include <sstream>
#include <string>   /* getline */
#include <vector>
#include <queue>
#include <iomanip>

using namespace std;
#define PTE_NUM 64  // a page table contains 64 PTE 
#define TAU 49 

int num_frames;        // number of frames
char *algo = NULL;     // paging algorithm
char *options = NULL;   // options
char *optarg;
unsigned long instr_count;
vector <int> randvals;

class Process;

typedef struct {
    Process* proc;
    int start_vpage;
    int end_vpage;
    bool write_protect;
    bool file_mapped; 
} vma;

typedef struct
{
    unsigned long maps = 0;
    unsigned long unmaps = 0;
    unsigned long ins = 0; 
    unsigned long fins = 0;
    unsigned long outs = 0;
    unsigned long fouts = 0;
    unsigned long zeros = 0;
    unsigned long segv = 0;
    unsigned long segprot = 0;
} pstatistics;

// PTE - page table entry
// All elements are initialized as 0 before simulation
typedef struct {
    bool cache_disabled = 0;
    bool modified = 0;
    bool referenced = 0;
    bool pageout = 0;
    bool file_mapped = 0;
    bool present = 0;
    bool write_protect = 0;
    unsigned frameNumber = 0;
    unsigned page_index = 0;
} pte_t; // can only be total of 32-bit size !!!!


class Process {
public:
    int num_vmas;
    vector<vma*> vma_vector;
    pte_t* page_table[PTE_NUM];
    unsigned proc_no;
    pstatistics* pstats = new pstatistics;
};

// frame table entry
typedef struct {
    int proc_no;
    bool free = true;
    unsigned frame_index;
    unsigned page_index;
    unsigned age_counter = 0;
    unsigned long tau;  // last instruction time
} frame_t;

frame_t** frametable;
int hand = 0;
Process* proc_arr;

// virtual base class
class Pager {
public:
    virtual frame_t* select_victim_frame() =0;
};

class FIFO: public Pager
{
public:
    frame_t* select_victim_frame() {   
        frame_t* temp = frametable[hand];
        hand = (hand + 1) % num_frames;
        return temp;
    }
};


class Random: public Pager
{
public:
    frame_t* select_victim_frame() {
        return frametable[myrandom()];
    }
    int myrandom() {
        static int offset = 1;
        int rand_count = randvals[0];
        int temp = randvals[offset] % num_frames;
        (offset < rand_count) ? offset++ : offset = 1;
        return temp;
    }
};

class Clock: public Pager
{
public:
    frame_t* select_victim_frame() {
        Process cur_procs = proc_arr[frametable[hand]->proc_no];
        pte_t* pte = cur_procs.page_table[frametable[hand]->page_index];
        while (pte->referenced == 1) {
            pte->referenced = 0;
            hand = (hand + 1) % num_frames;
            cur_procs = proc_arr[frametable[hand]->proc_no];
            pte = cur_procs.page_table[frametable[hand]->page_index];
        }
        frame_t* temp = frametable[hand];
        hand = (hand + 1) % num_frames;
        return temp;
    }
};

class NRU: public Pager
{
    int last_instr_count = 0; //instructions since the last time the reference bits were reset
public:
    frame_t* select_victim_frame() {
        
        int instr_number = instr_count - last_instr_count + 1;
        int search_pointer = hand;
        // just record the first frame of each class
        int class0 = -1;
        int class1 = -1;
        int class2 = -1;
        int class3 = -1;
        pte_t* pte;
        
        for (int i = 0; i < num_frames; i++) {
            Process cur_procs = proc_arr[frametable[search_pointer]->proc_no];
            pte = cur_procs.page_table[frametable[search_pointer]->page_index];
            if (!pte->referenced){
                if (!pte->modified){
                    if (instr_number <= TAU){
                        hand = (search_pointer + 1) % num_frames;
                        return frametable[search_pointer];
                    }
                    else{
                        class0 = search_pointer;
                        break; 
                    }
                }
                else{
                    if (pte->modified && class1 == -1) class1 = search_pointer;
                }
            }
            else{
                if (!pte->modified && class2 == -1) class2 = search_pointer;
                if (pte->modified && class3 == -1) class3 = search_pointer;
            }
            search_pointer = (search_pointer + 1) % num_frames;
        }

        if (instr_number > TAU) {
            // need to reset R bit for all frames after consider their class
            for (int i = 0; i < num_frames; i++)
                proc_arr[frametable[i]->proc_no].page_table[frametable[i]->page_index]->referenced = 0;
            last_instr_count = instr_count + 1;
        }
        
        int temp;
        if (class3 != -1) temp = class3;
        if (class2 != -1) temp = class2;
        if (class1 != -1) temp = class1;
        if (class0 != -1) temp = class0;
        hand = (temp + 1) % num_frames;
        return frametable[temp];         
    }
};

class Aging: public Pager
{
public:
    frame_t* select_victim_frame() {
        int search_pointer = hand;
        int temp = search_pointer;
        
        pte_t* pte;

        for (int i =0; i < num_frames; i++){
            frametable[search_pointer]->age_counter /= 2;
            Process cur_proc = proc_arr[frametable[search_pointer]->proc_no];
            pte = cur_proc.page_table[frametable[search_pointer]->page_index];            
            frametable[search_pointer]->age_counter += pte->referenced * pow(2,31);
            if (frametable[search_pointer]->age_counter < frametable[temp]->age_counter)
                temp = search_pointer;
            search_pointer = (search_pointer + 1) % num_frames;
        }
        hand = (temp + 1) % num_frames;

        for (int i = 0; i < num_frames; i++)
            proc_arr[frametable[i]->proc_no].page_table[frametable[i]->page_index]->referenced = 0;
        
        return frametable[temp];
    }
};

class WorkingSet: public Pager
{
public:
    frame_t* select_victim_frame() {
        int search_pointer = hand;
        int reset_count = 0;
        int temp = -1;
        int oldest_frame = -1;
        int smallest_tau;
        int oldest_frame_1 = -1;
        int smallest_tau_1; 
        int max_count;
        pte_t* pte;

        for (int i = 0; i < num_frames; i++){
            Process cur_procs = proc_arr[frametable[search_pointer]->proc_no];
            pte = cur_procs.page_table[frametable[search_pointer]->page_index];
            if (pte->referenced == 1) {
                reset_count++;
                if (oldest_frame == -1) {
                    oldest_frame = search_pointer;
                    smallest_tau = frametable[search_pointer]->tau;
                }
            }
            else {
                if (instr_count > TAU + frametable[search_pointer]->tau) {
                    temp = search_pointer;
                    max_count = i;
                    break;
                } 
                else if (instr_count <= TAU + frametable[search_pointer]->tau && oldest_frame_1 == -1){
                    oldest_frame_1 = search_pointer;
                    smallest_tau_1 = frametable[search_pointer]->tau;
                }
            }
            search_pointer = (search_pointer + 1) % num_frames;
        }

        search_pointer = hand;
        if (temp != -1) {
            for (int i = 0; i < max_count; i++) {
                Process cur_procs = proc_arr[frametable[search_pointer]->proc_no];
                pte = cur_procs.page_table[frametable[search_pointer]->page_index];
                if (reset_count <= num_frames && pte->referenced == 1) {
                    frametable[search_pointer]->tau = instr_count;
                    pte->referenced = 0;
                }
                search_pointer = (search_pointer + 1) % num_frames;    
            }
        }
        else{
            if (reset_count == num_frames){ //reset all frames
                for (int i = 0; i < num_frames; i++) {
                    proc_arr[frametable[search_pointer]->proc_no].page_table[frametable[search_pointer]->page_index]->referenced = 0;
                    if (reset_count == num_frames)  frametable[search_pointer]->tau = instr_count;
                    if (frametable[search_pointer]->tau < smallest_tau) {
                        oldest_frame = search_pointer;
                        smallest_tau = frametable[search_pointer]->tau;
                    }
                    search_pointer = (search_pointer + 1) % num_frames;
                }
                temp = oldest_frame;
            }
            else {    
                for (int i = 0; i < num_frames; i++) { //reset the referenced frames
                    Process cur_proc = proc_arr[frametable[search_pointer]->proc_no];
                    pte = cur_proc.page_table[frametable[search_pointer]->page_index];
                    if (pte->referenced == 1) {
                        frametable[search_pointer]->tau = instr_count;
                        pte->referenced = 0;
                    } 
                    else if (pte->referenced == 0 && frametable[search_pointer]->tau < smallest_tau_1) {
                        oldest_frame_1 = search_pointer;
                        smallest_tau_1 = frametable[search_pointer]->tau;
                    }
                    search_pointer = (search_pointer + 1) % num_frames;
                }
                temp = oldest_frame_1;
            }
        }
        hand = (temp + 1) % num_frames;
        return frametable[temp];
    }
};

// Initializing a free list to store free frames.
// Starting paging when we run out of free frames
queue<frame_t*> freeframelist;
frame_t* allocate_frame_from_free_list() {
    frame_t* temp = NULL;
    if (!freeframelist.empty()) {
        temp = freeframelist.front();
        freeframelist.pop();
    }
    return temp;
}

Pager *THE_PAGER;
frame_t *get_frame() {
    frame_t *frame = allocate_frame_from_free_list();   
    if (frame == NULL) frame = THE_PAGER->select_victim_frame();
    return frame;
}

// Here starts the simulation
int main(int argc, char ** argv) {

    int c;
    while ((c = getopt(argc, argv, "f:a:o:")) != -1) {
        if (c=='f') num_frames = atoi(optarg);
        else if (c=='o') options = optarg;
        else if (c=='a') algo = optarg;
        else cout<<"Wrong CMD Input!";
    }

    frametable = new frame_t*[num_frames];
    for (int i = 0; i < num_frames; i++) { 
    //initializing all frames in frametable
        frametable[i] = new frame_t;
        frametable[i]->frame_index = i;
        freeframelist.push(frametable[i]);
    }

    if (!strcmp(algo, "f")) THE_PAGER = new FIFO();
    else if (!strcmp(algo, "r")) THE_PAGER = new Random();
    else if (!strcmp(algo, "c")) THE_PAGER = new Clock();
    else if (!strcmp(algo, "e")) THE_PAGER = new NRU();
    else if (!strcmp(algo, "a")) THE_PAGER = new Aging();
    else if (!strcmp(algo, "w")) THE_PAGER = new WorkingSet();

    bool option_O = false, option_P = false, option_F = false, option_S = false, 
    option_x = false, option_y = false, option_f = false, option_a = false;
    for(int i=0; i<strlen(options); i++) {
        if (options[i]=='O') option_O = true;
        else if (options[i]=='P') option_P = true;
        else if (options[i]=='F') option_F = true;
        else if (options[i]=='S') option_S = true;
        else if (options[i]=='x') option_x = true;
        else if (options[i]=='y') option_y = true;
        else if (options[i]=='f') option_f = true;
        else if (options[i]=='a') option_a = true;
    }
    
    // read randfile
    ifstream randfile(argv[optind + 1]);
    int temp;
    randfile >> temp;
    randvals.push_back(temp);
    for (int i = 1; i < randvals[0]; i++) {
        randfile >> temp;
        randvals.push_back(temp);
    }

    // read input file
    ifstream inputfile(argv[optind]);
    string line;
    int num_procs = -1;  
    int num_vmas = -1; // record the number of processes and its vmas
    int cur_procs, cur_vmas;
    // vmas for each process
    while (getline(inputfile, line)) {
        if (line[0]!='#'){ // ALL lines starting with '#' must be ignored
            istringstream iss(line); 
            if (num_procs == -1) {
                iss >> num_procs;
                proc_arr = new Process[num_procs];
                for (int i = 0; i < num_procs; i++) {
                    proc_arr[i] = Process();
                    proc_arr[i].proc_no = i;
                }
                cur_procs = 0;
            }

            else if (num_vmas == -1) {              
                iss >> num_vmas;
                proc_arr[cur_procs].num_vmas = num_vmas;
                proc_arr[cur_procs].proc_no = cur_procs;
                cur_vmas = 0;
            }

            else if (cur_vmas < num_vmas) {
                proc_arr[cur_procs].vma_vector.push_back(new vma);
                iss >> proc_arr[cur_procs].vma_vector.back()->start_vpage 
                >> proc_arr[cur_procs].vma_vector.back()->end_vpage
                >> proc_arr[cur_procs].vma_vector.back()->write_protect 
                >> proc_arr[cur_procs].vma_vector.back()->file_mapped;
                cur_vmas++;
                if (cur_vmas >= num_vmas) {
                    cur_procs++;
                    num_vmas = -1;
                }
                if (cur_procs == num_procs) break;
            }
        }        
    }
    
    
    for (int i = 0; i < num_procs; i++) {
        for (int j = 0; j < PTE_NUM; j++) {
            proc_arr[i].page_table[j] = new pte_t;
        }        
    }
    
    instr_count = 0;
    unsigned long ctx_switches = 0;
    unsigned long process_exits = 0;
    unsigned long long cost = 0;
    char instr;
    int operand;
    Process* cur_proc = nullptr;

    // instructions
    while (getline(inputfile, line)) {
        if (line[0]!='#'){
            //cout << line <<"\n";
            istringstream iss(line);
            pte_t* pte;
            
            iss >> instr >> operand;
            (option_O) ? cout << instr_count << ": ==> " << instr << " " << operand << "\n" : cout << "";
            
            if (instr == 'c'){
                // context switch to process #<procid> and write its vmas
                cur_proc = &(proc_arr[operand]);
                ctx_switches++;
                cost += 121;
                for (int k = 0; k < cur_proc->vma_vector.size(); k++){
                    for (int j = cur_proc->vma_vector[k]->start_vpage; j <= cur_proc->vma_vector[k]->end_vpage; j++){
                        cur_proc->page_table[j]->write_protect = cur_proc->vma_vector[k]->write_protect;
                        cur_proc->page_table[j]->file_mapped = cur_proc->vma_vector[k]->file_mapped;
                    }
                }
            }
            else if (instr == 'w' || instr == 'r'){
                cost += 1;
                pte = cur_proc->page_table[operand];
                pte->page_index = operand;

                if (!pte->present) {                       
                    // page fault
                    bool SEGV_Error = true;
                    for (int k = 0; k < cur_proc->vma_vector.size(); k++){
                        if (operand >= cur_proc->vma_vector[k]->start_vpage 
                        && operand <= cur_proc->vma_vector[k]->end_vpage){
                            SEGV_Error = false;
                            break;
                        }
                    }

                    if (SEGV_Error) { //handling SEGV
                        instr_count++;
                        cur_proc->pstats->segv++;
                        cost += 240;
                        if (option_O) cout << " SEGV\n";
                        continue;
                    }

                    frame_t* newframe = get_frame();

                    if (!newframe->free) { // need to unmap the victim frame
                        pte_t* prev_pte = proc_arr[newframe->proc_no].page_table[newframe->page_index];
                        proc_arr[newframe->proc_no].pstats->unmaps++;
                        cost += 400;
                        if (option_O) cout << " UNMAP " << newframe->proc_no << ":" << prev_pte->page_index << "\n";
                        
                        if (prev_pte->modified) { // the frame is dirty 
                            if (prev_pte->file_mapped){
                                proc_arr[newframe->proc_no].pstats->fouts++;
                                cost += 2500;
                                if (option_O) cout << " FOUT\n";
                            }
                            else{
                                proc_arr[newframe->proc_no].pstats->outs++;
                                cost += 3000;
                                prev_pte->pageout = 1;
                                if (option_O) cout << " OUT\n";
                            }
                        } 
                        // reset the PTE
                        prev_pte->present = 0;
                        prev_pte->modified = 0;
                    }

                    if (pte->file_mapped) { 
                        cur_proc->pstats->fins++;
                        cost += 2500;
                        if (option_O) cout << " FIN\n";
                    } 
                    else if (pte->pageout) {
                        cur_proc->pstats->ins++;
                        cost += 3000;
                        if (option_O) cout << " IN\n";
                    } 
                    else {
                        cur_proc->pstats->zeros++;
                        cost += 150;
                        if (option_O) cout << " ZERO\n";
                    }
                    newframe->free = false;
                    newframe->proc_no = cur_proc->proc_no;
                    newframe->page_index = operand;
                    newframe->age_counter = 0;
                    newframe->tau = instr_count;
                    pte->frameNumber = newframe->frame_index;
                    pte->present = 1;
                    cur_proc->pstats->maps++;
                    cost += 400;
                    if (option_O) cout << " MAP " << newframe->frame_index << "\n";
                }
                pte->referenced = 1;
                if (instr == 'w') {
                    pte->modified = 1;
                    if (pte->write_protect) {
                        cur_proc->pstats->segprot++;
                        cost += 300;
                        pte->modified = 0;
                        if (option_O) cout << " SEGPROT\n";
                    }
                }
            }
            else if (instr == 'e'){
                cout << "EXIT current process " << cur_proc->proc_no << "\n";
                process_exits++;
                cost += 175;

                for (int k = 0; k < cur_proc->vma_vector.size(); k++){
                    for (int i = cur_proc->vma_vector[k]->start_vpage; i <= cur_proc->vma_vector[k]->end_vpage; i++){
                        if (cur_proc->page_table[i]->present) {
                            cur_proc->page_table[i]->present = 0;
                            cur_proc->pstats->unmaps++;
                            cost += 400;
                            if (option_O) cout << " UNMAP " << cur_proc->proc_no << ":" << i << "\n";
                            if (cur_proc->page_table[i]->file_mapped && 
                                cur_proc->page_table[i]->modified) {
                                cur_proc->pstats->fouts++;
                                cost += 2500;
                                if (option_O) cout << " FOUT\n";
                            }
                            // use frametable to free frame reversely
                            frametable[cur_proc->page_table[i]->frameNumber]->free = 1;
                            freeframelist.push(frametable[cur_proc->page_table[i]->frameNumber]);
                        }
                        cur_proc->page_table[i]->pageout = 0;
                    }

                }
            }
            instr_count++;

            if (option_x) {
                cout << "PT[" << cur_proc->proc_no << "]: ";
                for (int i = 0; i < PTE_NUM; i++) {
                    pte_t* pte = cur_proc->page_table[i];
                    if (!pte->present) {
                        (!pte->pageout) ? cout << "*" : cout << "#";
                    } 
                    else {
                        cout << i << ":";
                        (pte->referenced) ? cout << "R" : cout << "-";
                        (pte->modified) ? cout << "M" : cout << "-";
                        (pte->pageout) ? cout << "S" : cout << "-";
                    }
                    cout << " ";
                }
                cout << endl;
            }

            if (option_y) {
                for (int i = 0; i < num_procs; i++) {
                    cout << "PT[" << i << "]: ";
                    for (int j = 0; j < PTE_NUM; j++) {
                        pte_t* pte = proc_arr[i].page_table[j];
                        if (pte->present) {
                            (pte->pageout) ? cout << "#" : cout << "*";
                        } 
                        else {
                            cout << j << ":";
                            (pte->referenced) ? cout << "R" : cout << "-";
                            (pte->modified) ? cout << "M" : cout << "-";
                            (pte->pageout) ? cout << "S" : cout << "-";
                        }
                        cout << " ";
                    }
                    cout << endl;
                }
            }              
        }
    }

    randfile.close();
    inputfile.close();

    if (option_P) {
        for (int i = 0; i < num_procs; i++) {
            cout << "PT[" << i << "]: ";
            for (int j = 0; j < PTE_NUM; j++) {
                pte_t* pte = proc_arr[i].page_table[j];
                if (!pte->present) {
                    (pte->pageout) ? cout << "#" : cout << "*";
                } 
                else {
                    cout << j << ":";
                    (pte->referenced) ? cout << "R" : cout << "-";
                    (pte->modified) ? cout << "M" : cout << "-";
                    (pte->pageout) ? cout << "S" : cout << "-";
                }
                cout << " "; 
            }
            cout << endl;
        }   
    }

    if (option_F) {
        cout << "FT: ";
        for (int i = 0; i < num_frames; i++) {
            (frametable[i]->free) ? cout << "* " : 
            cout << frametable[i]->proc_no << ":" 
            << frametable[i]->page_index << " ";
        }
        cout << "\n";
    }

    if (option_S) {
        for (int i = 0; i < num_procs; i++) {
            Process *proc = &proc_arr[i];
            pstatistics *pstats = proc->pstats;
            printf("PROC[%d]: U=%lu M=%lu I=%lu O=%lu FI=%lu FO=%lu Z=%lu SV=%lu SP=%lu\n",
                proc->proc_no,
                pstats->unmaps, pstats->maps, pstats->ins, pstats->outs,
                pstats->fins, pstats->fouts, pstats->zeros,
                pstats->segv, pstats->segprot);
        }
        printf("TOTALCOST %lu %lu %lu %llu\n", instr_count, ctx_switches, process_exits, cost);
    }

}