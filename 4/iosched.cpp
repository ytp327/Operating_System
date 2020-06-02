#include <iostream>
#include <fstream>
#include <queue>
#include <vector>
#include <unistd.h>  // getopt
#include <stdlib.h>  // abs
#include <algorithm>    // max
#include <limits.h>
#include <string>
#include <sstream>

using namespace std;
char *optarg;

int head = 0;         // disk head
int direction = 1;   // disk head direction

struct iorequest
{
    int io_op_number;
    int track_number;
    int arrival_time;   // request arrive time
    int start_time;     // request start time
    int end_time;       // request end time
};

queue<iorequest*> input_queue;
vector<iorequest*> io_print_list; // used for printing results
vector<iorequest*> io_wait_list;

class Scheduler {
public:
    virtual iorequest* strategy() {}
};

Scheduler *THE_SCHEDULER;
class FIFO: public Scheduler
{
public:
    iorequest* strategy() {
        iorequest* res = io_wait_list.front();
        (res->track_number > head) ? direction = 1 : direction = -1;
        io_wait_list.erase(io_wait_list.begin());
        return res;
    }
};

class SSTF: public Scheduler
{
public:
    iorequest* strategy() {
        iorequest* res = io_wait_list.front();
        int pointer = 0;
        direction = res->track_number - head >= 0 ? 1 : -1;
        for (int i = 0; i < io_wait_list.size(); i++) {
            if (abs(res->track_number - head) > abs(io_wait_list[i]->track_number - head)) {
                pointer = i;
                res = io_wait_list[i];
                direction = io_wait_list[i]->track_number - head >= 0 ? 1 : -1;
            }
        }
        io_wait_list.erase(io_wait_list.begin() + pointer);
        return res;
    }
};

class LOOK: public Scheduler
{
public:
    iorequest* strategy() {
        iorequest* res = NULL;
        int next_dis = INT_MAX;
        int pointer = 0;
        for (int i = 0; i < io_wait_list.size(); i++) {
            if ((io_wait_list[i]->track_number - head) * direction >= 0 && 
            next_dis > abs(io_wait_list[i]->track_number - head)) {
                next_dis = abs(io_wait_list[i]->track_number - head);
                res = io_wait_list[i];
                pointer = i;
            }
        }
        if (res) {
            io_wait_list.erase(io_wait_list.begin() + pointer);
            return res;
        }
        else { //change direction
            direction = - direction;
            return res = strategy();
        }
    }
};

// back to the start
class CLOOK: public Scheduler
{
public:
    iorequest* strategy() {
        iorequest* res = NULL;
        iorequest* start = NULL;
        int pointer = 0, start_pointer = 0;
        int next_dis = INT_MAX, start_track = INT_MAX;
        if (direction == -1) { // reverse direction
            direction = 1;
            res = strategy();
        }
        else {
            for (int i = 0; i < io_wait_list.size(); i++) {
                if (io_wait_list[i]->track_number < start_track) {
                    start_track = io_wait_list[i]->track_number;
                    start = io_wait_list[i];
                    start_pointer = i;
                }
                if (io_wait_list[i]->track_number - head >= 0) {
                    if (next_dis > abs(io_wait_list[i]->track_number - head)) {
                        next_dis = abs(io_wait_list[i]->track_number - head);
                        res = io_wait_list[i];
                        pointer = i;
                    }
                }
            }
            if (res == NULL) {
                direction = -1;
                res = start;
                pointer = start_pointer;
            }
            io_wait_list.erase(io_wait_list.begin() + pointer);
        }
        return res;   
    }
};

vector<iorequest*> active_queue;
class FLOOK: public Scheduler
{
public:    
    iorequest* strategy() {
        int pointer = 0;
        iorequest* res = NULL;
        if (active_queue.empty() and !io_wait_list.empty()) {
            //when active-queue is empty, the queues are swapped
            active_queue.swap(io_wait_list);
        }
        
        int next_dis = INT_MAX;
        for (int i = 0; i < active_queue.size(); i++) {
            if ((active_queue[i]->track_number - head) * direction >= 0
            && next_dis > abs(active_queue[i]->track_number - head)) {
                next_dis = abs(active_queue[i]->track_number - head);
                res = active_queue[i];
                pointer = i;
            }
        }
        if (res) {
            active_queue.erase(active_queue.begin() + pointer);
            return res;
        }
        if (!active_queue.empty()) { //change direction in active queue
            direction = - direction;
            res = strategy();
        }
        return res;       
    }
};

int main(int argc, char ** argv) {
	char* algo = NULL;   // schedule algorithm

	if (getopt(argc, argv, "s:") != -1) {
        algo = optarg;
	}

    if (*algo == 'i')  THE_SCHEDULER = new FIFO();
    else if (*algo == 'j') THE_SCHEDULER = new SSTF();
    else if (*algo == 's') THE_SCHEDULER = new LOOK();
    else if (*algo == 'c') THE_SCHEDULER = new CLOOK();
    else if (*algo == 'f') THE_SCHEDULER = new FLOOK();

    // read input file
    ifstream inputfile(argv[optind]);
    string line;
    int req_count = 0;  // request counter
    
    // read io requests, push to input_queue and record in io_print_list
    while (getline(inputfile, line)) {
        if (line[0] != '#') {
            istringstream iss(line);
            iorequest* req = new iorequest;
            req->io_op_number = req_count;
            iss >> req->arrival_time;
            iss >> req->track_number;
            input_queue.push(req);
            io_print_list.push_back(req);
            req_count++;
        }
    }
    
    // simulation
    int timer = 0, tot_movement = 0;
    iorequest* current_io_req = NULL;

    while (!input_queue.empty() || !io_wait_list.empty() || current_io_req) {
        // io request come
        if (!input_queue.empty() && input_queue.front()->arrival_time == timer) {
            io_wait_list.push_back(input_queue.front());
            input_queue.pop();
        }
        
        if (current_io_req) {
            if (head == current_io_req->track_number){ // io request finish
                current_io_req->end_time = timer;
                current_io_req = NULL;
            }
            else{ // not finish, move header
                head += direction;
                tot_movement++;
            }
        }
        
        // current_io_req is NULL and need to schedule new frame 
        while (!current_io_req && !(io_wait_list.empty() && active_queue.empty())) {
            current_io_req = THE_SCHEDULER->strategy();
            if (current_io_req){
                current_io_req->start_time = timer;
                if (head == current_io_req->track_number) {
                    current_io_req->end_time = timer;
                    current_io_req = NULL;
                } else {
                    head += direction;
                    tot_movement++;
                }
            }
        }
        timer++;
    }

    double avg_turnaround = 0, avg_waittime = 0;
    int max_waittime = 0;

    for (int i = 0; i < req_count; i++) {
        printf("%5d: %5d %5d %5d\n", i, io_print_list[i]->arrival_time, 
            io_print_list[i]->start_time, io_print_list[i]->end_time);
        avg_turnaround += io_print_list[i]->end_time - io_print_list[i]->arrival_time;
        avg_waittime += io_print_list[i]->start_time - io_print_list[i]->arrival_time;
        max_waittime = max(max_waittime, io_print_list[i]->start_time - io_print_list[i]->arrival_time);
    }

    avg_turnaround = avg_turnaround / req_count;
    avg_waittime = avg_waittime / req_count;
    printf("SUM: %d %d %.2lf %.2lf %d\n", timer - 1, tot_movement, avg_turnaround,
        avg_waittime, max_waittime);
    delete THE_SCHEDULER;
}