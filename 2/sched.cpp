  #include <iostream>
  #include <fstream>
  #include <string>
  #include <sstream>
  #include <vector>
  #include <list>
  #include <queue>
  #include <unistd.h>
  #include <ctype.h>
  #include <limits.h>
  #include <string.h>
  #include <algorithm>
  #include <stdio.h>
  #include <stdlib.h>
  using namespace std;

  int id = 0;
  int maxprio=4;
  char *stype;
  pair<int, int> pairT;
  vector <int> randvals;
  class Event;
  enum State {CREATED, READY, RUNNING, BLOCKED, DONE};
  enum TranState {TRANS_TO_READY, TRANS_TO_RUN, TRANS_TO_BLOCK, TRANS_TO_DONE, TRANS_TO_PREEMPT};
  int getEventID() {
      return id++;
  }
  // STEP 1: read input file to create Process
  // AT TC CB IO
  // AT: arrival time; TC: total CPU time; CB: CPU burst; IO: IO burst
  int myrandom(int burst) {
    static int ofs = 1;
    int res;
    // wrap around or step to the next
    if (ofs>randvals[0]){
      ofs = 1;
      res = (randvals[ofs] % burst) + 1;
    }
    else{
      res =(randvals[ofs++] % burst) + 1;
    }
    return res;
  }

  class Process
  {
  public:
    int at, tc, cb, io;
    int cpuBurst, ioBurst;
    State curState = CREATED; // current state
    int timeRemaining; // remaining time
    int timeInPrevState = 0;
    int curStateTime; // time when the process starts current state
    int finishingTime; // finishing time
    int ioTime = 0; // time in blocked state
    int cpuWaitingTime = 0; // time in Ready state
    int static_priority, dynamic_priority;
    bool preempted = 0, dynamic_priority_reset = 0;
    Event* pendingEvt = nullptr;

    Process(int a, int t, int c, int i)
    {        
      at = a, tc = t, cb = c, io = i;
      timeRemaining = tc;
      curStateTime = at;
      static_priority = myrandom(maxprio);
      dynamic_priority = static_priority - 1;
    }

    void updateTimeRemaining() {
      timeRemaining = (timeInPrevState == 0) ? tc : timeRemaining-timeInPrevState;
    }

    void updateCPUBurst() {
      cpuBurst = min(timeRemaining, myrandom(cb));
    }
    void update_dynamic_prio() {
          // if quantum expires
          dynamic_priority -= 1;
          if (dynamic_priority == -1) {
              dynamic_priority = static_priority - 1;
              dynamic_priority_reset = 1;
          } else {
              dynamic_priority_reset = 0;
          }  
      }
  };


  // STEP 2: define DES and event to fire process

  class Event
  {
  public:
      int eventID, evtTimeStamp;
      Process * evtProcess;
      TranState transition;
      bool deleted = false;

      Event(int timestamp, Process *proc, TranState tran, int eventID): eventID(eventID) {
        evtTimeStamp = timestamp;
        evtProcess = proc;
        transition = tran;
      }   
  };

  class EventCompare
  {
    public:
      bool operator() (Event* e1, Event* e2) {
        if (e1->evtTimeStamp == e2->evtTimeStamp) return e1->eventID > e2->eventID;
        return e1->evtTimeStamp > e2->evtTimeStamp;
      }
  };

  class DES
  {
  public:
    priority_queue<Event*, vector<Event*>, EventCompare> eventQueue;
    Event* get_event() {
        if (eventQueue.empty()) {
            return nullptr;
        }
        return eventQueue.top();
    }

    void add_event(Event *evt) {
        eventQueue.push(evt);
    }

    void rm_event() {
        eventQueue.pop();
    }
  };

  // STEP 3: implement schedulers

  class Scheduler
  {
  public:
    int quantum = 10000; // set a large defualt quantum

    // virtual function
    virtual void add_process(Process *proc) {}
    virtual Process* get_next_process() {}
    virtual bool noProcess(){}
  };


  // FCFS: first come, first served
  // LCFS: last come, first served
  // SRTF: shortest remaining time first
  // RR (Round Robin): cyclic executive without priority
  // PRIO (Priority scheduler): RR + priority but not preemptive
  // Preemptive PRIO (E): preemptive PRIO, a ready process with higher priority can preempt CPU to run

  class FCFS: public Scheduler
  {
    queue<Process*> runQueue;
  public:
    void add_process(Process *proc) {
        runQueue.push(proc);
    }
    Process* get_next_process() {
        Process * temp = runQueue.front();
        runQueue.pop();
        return temp;
    }
    bool noProcess() {return runQueue.empty();}
  };

  class LCFS: public Scheduler
  {
    list<Process*> runQueue;
  public:

    void add_process(Process *proc) {
      runQueue.push_back(proc);
    }
    Process* get_next_process() {
      Process* temp = runQueue.back();
      runQueue.pop_back();
      return temp;
    }
    bool noProcess() {return runQueue.empty();}
  };

  class SRTF_compare
  {
  public:
      bool operator() (Process* p1, Process* p2) {
        if (p1->timeRemaining == p2->timeRemaining) return p1 > p2;
        return p1->timeRemaining > p2->timeRemaining;
      }
  };
  class SRTF: public Scheduler
  { // non-preemptive version
    priority_queue<Process*, vector<Process*>, SRTF_compare> runQueue;
  public:
    void add_process(Process *proc) {
      runQueue.push(proc);
    }
    Process* get_next_process() {
      Process * temp = runQueue.top();
      runQueue.pop();
      return temp;
    }
    bool noProcess() {return runQueue.empty();}
  };

  class RR: public Scheduler
  {
    queue <Process*> runQueue;
  public:  
    RR(int quan) {
        quantum = quan;
    }

    void add_process(Process *proc) {
        runQueue.push(proc);    
    }

    Process* get_next_process() {
        Process * temp = runQueue.front();
        runQueue.pop();
        return temp;
    }
    bool noProcess() {return runQueue.empty();}
  };

  class PRIO: public Scheduler
  {  
    int AQCount=0, EQCount=0;
    vector <queue<Process*>> activeQueue = vector <queue<Process*>> (maxprio);
    vector <queue<Process*>> expiredQueue = vector <queue<Process*>> (maxprio);
    public: 
      PRIO(int quan) {
          quantum = quan;       
      }
      void add_process(Process *proc) { 
          if (proc->curState == CREATED || proc->curState == BLOCKED) {
            activeQueue[proc->dynamic_priority].push(proc);
            AQCount+=1;  
          } else if (proc->curState == RUNNING) {
              // quantum expires
            if (!proc->dynamic_priority_reset) {
              activeQueue[proc->dynamic_priority].push(proc);
              AQCount+=1;
            } else {
              expiredQueue[proc->dynamic_priority].push(proc);
              EQCount+=1;
            }
            proc->dynamic_priority_reset = 0;
          }
      }

      void ActExpExchange() {
        if (AQCount!=0) return;
        queue<Process*> temp;
        for (int i = 0; i < maxprio; i++) {
          temp = activeQueue[i];
          activeQueue[i] = expiredQueue[i];
          expiredQueue[i] = temp;
        }
        int tempI; tempI=AQCount; 
        AQCount=EQCount; EQCount=tempI;
      }

      Process* get_next_process() {
          // when the active queue is empty, switch active and expired
          int curMaxPrio = maxprio-1;
          while (curMaxPrio > -1) {
            if (activeQueue[curMaxPrio].empty()){ curMaxPrio-=1; }
            else{
              Process* temp = activeQueue[curMaxPrio].front();
              activeQueue[curMaxPrio].pop();
              AQCount-=1;
              return temp;
            }
          } 
      }
      bool noProcess(){ 
        if (AQCount==0) ActExpExchange();
        return AQCount==0;
      }
  };
  
  void Simulation(DES *desLayer, vector<pair<int, int> > &IOUse) {
    Scheduler* THE_SCHEDULER;
    int quantum, maxprio;
      if (strcmp(stype, "F") == 0) {
        THE_SCHEDULER = new FCFS();
        cout << "FCFS" << "\n";
      } else if (strcmp(stype, "L") == 0) {
          THE_SCHEDULER = new LCFS();
          cout << "LCFS" << "\n";
      } else if (strcmp(stype, "S") == 0) {
          THE_SCHEDULER = new SRTF();
          cout << "SRTF" << "\n";
      } else if (*stype == 'R') {
          quantum = atoi(stype + 1);
          THE_SCHEDULER = new RR(quantum);
          cout << "RR" << " " << THE_SCHEDULER->quantum << "\n";
      } else if (*stype == 'P') {
          sscanf(stype + 1, "%d:%d", &quantum, &maxprio);
          THE_SCHEDULER = new PRIO(quantum);
          cout << "PRIO" << " " << THE_SCHEDULER->quantum << "\n";
      } else if (*stype == 'E') {
          sscanf(stype + 1, "%d:%d", &quantum, &maxprio);
          THE_SCHEDULER = new PRIO(quantum);
          cout << "PREPRIO" << " " << THE_SCHEDULER->quantum << "\n";
      }
      
      Event* evt;
      bool CALL_SCHEDULER;
      Process * CURRENT_RUNNING_PROCESS = nullptr;

      while (evt = desLayer->get_event()) {
        desLayer->rm_event();
        if (evt->deleted == true) continue;
        
        Process *proc = evt->evtProcess; Event *newEvt;
        proc->timeInPrevState = evt->evtTimeStamp - proc->curStateTime;
        proc->curStateTime = evt->evtTimeStamp;
        int CURRENT_TIME = evt->evtTimeStamp;
        
        switch(evt->transition) { 
            case TRANS_TO_READY:
              if(!(proc->curState == BLOCKED || proc->curState == CREATED || proc->curState == RUNNING)) {
                //cout<<"ERROR IN TRANS!";
              }
                // must come from BLOCKED or from PREEMPTION
                if (proc->curState == BLOCKED) {
                  proc->ioTime += proc->timeInPrevState;
                  proc->ioBurst -= proc->timeInPrevState;
                  proc->dynamic_priority = proc->static_priority-1;
                }
                // must add to run_queue
                THE_SCHEDULER->add_process(proc);                 
                proc->curState = READY;
                CALL_SCHEDULER = true;
                break;

            case TRANS_TO_RUN:
              if (proc->curState!=READY) {//cout<<"ERROR IN TRANS!";
              }
              if (proc->preempted == 0) {
                proc->cpuBurst = min(myrandom(proc->cb), proc->timeRemaining);
              }
              else {proc->preempted = 0;}
              CURRENT_RUNNING_PROCESS = proc;
              if (proc->curState==READY) proc->cpuWaitingTime += proc->timeInPrevState;
              proc->curState = RUNNING;
              if (proc->cpuBurst <= THE_SCHEDULER->quantum){
                if (proc->timeRemaining <= proc->cpuBurst){
                  newEvt = new Event(CURRENT_TIME + proc->timeRemaining, proc, TRANS_TO_DONE, getEventID());
                }
                else{
                  newEvt = new Event(CURRENT_TIME + proc->cpuBurst, proc, TRANS_TO_BLOCK, getEventID());
                }
              }
              else{
                  newEvt = new Event(CURRENT_TIME + THE_SCHEDULER->quantum, proc, TRANS_TO_PREEMPT, getEventID());
                  proc->preempted = 1;
              }

              proc->pendingEvt = newEvt;
              desLayer->add_event(newEvt);
              break;

            case TRANS_TO_BLOCK:
              if (proc->curState!=RUNNING) {
                //cout<<"ERROR IN TRANS!";
              }
                CURRENT_RUNNING_PROCESS = nullptr;
                proc->ioBurst = myrandom(proc->io); 
                proc->cpuBurst -= proc->timeInPrevState;
                proc->timeRemaining -= proc->timeInPrevState;
                proc->curState = BLOCKED;

                pairT.first=CURRENT_TIME;
                pairT.second=CURRENT_TIME + proc->ioBurst;
                IOUse.push_back(pairT);
                
                newEvt = new Event(CURRENT_TIME + proc->ioBurst, proc, TRANS_TO_READY, getEventID());
                desLayer->add_event(newEvt);
                proc->pendingEvt = newEvt;
                CALL_SCHEDULER = true;
                break;

            case TRANS_TO_PREEMPT:
              if (proc->curState!=RUNNING){
                //cout<<"ERROR IN TRANS!";
              } 
              proc->timeRemaining -= proc->timeInPrevState;
              proc->cpuBurst -= min(quantum, proc->timeInPrevState);
              CURRENT_RUNNING_PROCESS = nullptr;
                
              proc->update_dynamic_prio();
              THE_SCHEDULER->add_process(proc);
              proc->curState = READY;
              CALL_SCHEDULER = true;
              break;

            case TRANS_TO_DONE: 
              if (CURRENT_RUNNING_PROCESS!=proc) continue;
              proc->timeRemaining = 0;
              proc->curState = DONE;
              CURRENT_RUNNING_PROCESS = nullptr;
              CALL_SCHEDULER = true;
              break;
          }

          delete evt;
          evt = nullptr;
          
          if (CALL_SCHEDULER) {
            if (CURRENT_RUNNING_PROCESS != nullptr && *stype=='E' &&CURRENT_RUNNING_PROCESS->dynamic_priority < proc->dynamic_priority&& CURRENT_RUNNING_PROCESS->pendingEvt->evtTimeStamp != CURRENT_TIME) {
              CURRENT_RUNNING_PROCESS->pendingEvt->deleted = true;
              newEvt = new Event(CURRENT_TIME, CURRENT_RUNNING_PROCESS, TRANS_TO_PREEMPT, getEventID());
              desLayer->add_event(newEvt);
              CURRENT_RUNNING_PROCESS->preempted = 1;
              continue;
              }

              Event* nextEvent= desLayer->get_event();
              if (nextEvent!=nullptr && nextEvent->evtTimeStamp == CURRENT_TIME) continue; 
              
              CALL_SCHEDULER = false; // reset global flag
              if (CURRENT_RUNNING_PROCESS == nullptr && !THE_SCHEDULER->noProcess()) {                    
                CURRENT_RUNNING_PROCESS = THE_SCHEDULER->get_next_process();
                  
                // run the process
                newEvt = new Event(CURRENT_TIME, CURRENT_RUNNING_PROCESS, TRANS_TO_RUN, getEventID());
                desLayer->add_event(newEvt);
                proc->pendingEvt = newEvt;
              }
          }
      }
      delete THE_SCHEDULER;
  }


  int main(int argc, char ** argv) {
      int vflag = 0;  // verbose
      int c, a;
      int index;
      
      while ((c = getopt(argc, argv, "vtes:")) != -1) {
          switch(c)
          {
            case 'v':
                vflag = 1;
                break;
            case 't': break;
            case 'e': break;
            case 's':
                stype = optarg;
                sscanf(stype+1, "%d:%d", &a, &maxprio);
                break;
          }
      }
      
      // read in randfile
      ifstream randfile(argv[optind + 1]);
      int temp;
      randfile >> temp;
      randvals.push_back(temp);
      for (int a = 1; a < randvals[0]; a++) {
          randfile >> temp;
          randvals.push_back(temp);
      }

      // read input file and create Process(at, tc, cb, io)
      ifstream inputfile;
      inputfile.open(argv[optind]);
      vector<Process> allProc;
      string line;
      getline(inputfile, line);
      
      while (line!=""){
        istringstream iss(line);
        string procInfo[4];
        iss>>procInfo[0]>>procInfo[1]>>procInfo[2]>>procInfo[3];
        
        Process process = Process(stoi(procInfo[0]), stoi(procInfo[1]), stoi(procInfo[2]), stoi(procInfo[3]));
        allProc.push_back(process);
        getline(inputfile, line);
      }
      inputfile.close();
      

      // put initial events for processes' arraival into the event queue 
      DES* desLayer = new DES();
      Event *newEvt;
      for (int i = 0; i<allProc.size(); ++i){
        newEvt = new Event(allProc[i].at, &allProc[i], TRANS_TO_READY, getEventID());
        desLayer->add_event(newEvt);
        allProc[i].pendingEvt = newEvt;
      }

      vector<pair<int, int> > IOUse; //[IOstart, IOend]
      // start simulation
      Simulation(desLayer, IOUse);

      int lastFT = 0;
      double cpuUti, ioUti, avgTT, avgCW, throughput;
      // print out statistics
      for (int i = 0; i< allProc.size(); i++){
        allProc[i].finishingTime=allProc[i].at+allProc[i].tc+allProc[i].ioTime+allProc[i].cpuWaitingTime;
        printf("%04d: %4d %4d %4d %4d %1d | %5d %5d %5d %5d\n",
        i, allProc[i].at, allProc[i].tc, allProc[i].cb, allProc[i].io,
        allProc[i].static_priority, allProc[i].finishingTime, 
        allProc[i].finishingTime-allProc[i].at, allProc[i].ioTime, 
        allProc[i].cpuWaitingTime);
        lastFT=max(lastFT, allProc[i].finishingTime);
        cpuUti += allProc[i].tc;
        ioUti += allProc[i].ioTime;
        avgTT += allProc[i].finishingTime - allProc[i].at;
        avgCW += allProc[i].cpuWaitingTime;
      }

      cpuUti = 100 * cpuUti / lastFT;
      
      sort(IOUse.begin(), IOUse.end());
      int totalIOUse = 0;
      if (!IOUse.empty()) {
          int start = IOUse[0].first, end = IOUse[0].second;
          for(auto x = IOUse.begin(); x!=IOUse.end(); ++x){
            if (end<x->first){
              totalIOUse += end - start;
              start = x->first;
              end = x->second;
            }
            else end = max(end, x->second);
          }
          totalIOUse += end - start;
      }
      ioUti = 100.0 * totalIOUse / lastFT;

      double count = allProc.size();
      avgTT /= count;
      avgCW /= count;
      throughput = 100.0 * allProc.size() / lastFT;
      printf("SUM: %d %.2lf %.2lf %.2lf %.2lf %.3lf\n", 
          lastFT, cpuUti, ioUti, avgTT, avgCW, throughput);
      return 0;
  }