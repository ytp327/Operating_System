#include <iostream>
#include <string.h>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <iterator>
#include <map>
#include<iomanip>
#include <typeinfo>
#include <algorithm>
using namespace std;

class tokenAndPosition{
  public:
    string token;
    int line;
    int position;
};
tokenAndPosition endOfFile;//store the end position of the file

vector<tokenAndPosition> tokenizer(const char * fileName) { 
  vector<tokenAndPosition> tokenAndPositions;
  string line,word,preline;
  int lineCounter=0;
  ifstream infile (fileName);

  if (infile.is_open()){
    while (getline (infile,line)){
      lineCounter++;
      preline=line;
      if (line.empty()) continue;
      int charCount=0;

      istringstream lineStream(line);
      for (int wordPosition = 0; lineStream >> word; wordPosition++) {
        tokenAndPosition currentToken;

        charCount=line.find(word,charCount);
        currentToken.token=word;currentToken.line=lineCounter;
        currentToken.position=charCount+1;
        charCount++;
        tokenAndPositions.push_back(currentToken);
      }
    }
    if(infile.eof()){
      endOfFile.token=infile.eof();endOfFile.line=lineCounter;
      endOfFile.position=preline.length()+1;
    }
  }
  else cout << "Unable to open file";
  infile.close();
  return tokenAndPositions; 
} 

string __parseerror(int errcode, int linenum, int lineoffset) {
  string errstr[] = {
    "NUM_EXPECTED", // Number expect
    "SYM_EXPECTED", // Symbol Expected
    "ADDR_EXPECTED", // Addressing Expected which is A/E/I/R
    "SYM_TOO_LONG", // Symbol Name is too long
    "TOO_MANY_DEF_IN_MODULE", // > 16
    "TOO_MANY_USE_IN_MODULE", // > 16
    "TOO_MANY_INSTR", // total num_instr exceeds memory size (512)
  };
  return "Parse Error line "+to_string(linenum)+" offset "+to_string(lineoffset)+": "+errstr[errcode]+"\n";
}

bool validSymbol(string sym){
  if(int(sym[0])<65||int(sym[0])>90&&int(sym[0])<97||int(sym[0])>122) return false;
  for(int i=1;i<sym.length();i++){
    if(int(sym[i])<48||int(sym[i])>57&&int(sym[i])<65||int(sym[i])>90&&int(sym[i])<97||int(sym[i])>122) return false;
  }
  return true;
}

bool validNumber(string num){//not correct
  for(int i=0;i<num.length();i++){
    if(int(num[i])<48||int(num[i])>57) return false;
  }
  return true;
}

vector<tokenAndPosition> modularizer(vector<tokenAndPosition> TAPs, int start, int moduleTag, int addressCounter){
  vector<tokenAndPosition> newTAP;
  try{
    if(!validNumber(TAPs[start].token)){
      cout<<__parseerror(0,TAPs[start].line,TAPs[start].position);
      exit(0);
    }
    int curModuleTokenCount=stoi(TAPs[start].token);
    
    if(addressCounter+curModuleTokenCount>512){
      cout<<__parseerror(6,TAPs[start].line,TAPs[start].position);
      exit(0);
    }
        
    if(moduleTag!=1) curModuleTokenCount*=2;
   for(int i=start;i-start<curModuleTokenCount+1;i++){
    newTAP.push_back(TAPs[i]);
   }
  }
  catch(...){}
  return newTAP;
}

int main(int argc, const char * argv[]) {

  // first pass
  vector<tokenAndPosition> tokenAndPositions=tokenizer(argv[1]);
  int tokenIndex=0;int moduleNum=1;int partNum=0;
  int addressCounter=0; //count the address up to now
  
  vector<string> moduleWarning; //store definition exceeds the size of the module waring
  map<string, int> symbolTable; //table to store symbols
  map<string, int> symbolUsing; //table to store symbols that have not been used up to now. Mapping symbol and its definition module
  map<string, string> symbolTableError;//table to store symbols multiple definition

  while(tokenIndex<tokenAndPositions.size()){
    vector<tokenAndPosition> tokens=modularizer(tokenAndPositions, tokenIndex, partNum, addressCounter);
    if (partNum==0){
      if (stoi(tokens[0].token)>16){
        cout<<__parseerror(4,tokens[0].line,tokens[0].position);
        return 0;
      }
      for(int i=1;i<tokens.size();i++){
        if(i%2==1){
          if(tokens[i].token.length()>16){
            cout<<__parseerror(3,tokens[i].line,tokens[i].position);
            return 0;
          }
          if(!validSymbol(tokens[i].token)){
            cout<<__parseerror(1,tokens[i].line,tokens[i].position);
            return 0;
          }
        }
        else{
          if(!validNumber(tokens[i].token)){
            cout<<__parseerror(0,tokens[i].line,tokens[i].position);
            return 0;
          }
        }
      }
      if((tokens.size()-1)/2<stoi(tokens[0].token)){
        if((tokens.size()-1)%2) cout<<__parseerror(0,endOfFile.line,endOfFile.position);
        else cout<<__parseerror(1,endOfFile.line,endOfFile.position);
        return 0;
      }

      for (int i=1;i<tokens.size();i+=2){
        if(symbolTable.find(tokens[i].token)!=symbolTable.end())
          symbolTableError[tokens[i].token]=tokens[i].token;
        else{
          symbolTable[tokens[i].token]=stoi(tokens[i+1].token)+addressCounter;
          symbolUsing[tokens[i].token]=moduleNum;
        }
      }
    }
    else if (partNum==1){
      if (stoi(tokens[0].token)>16){
          cout<<__parseerror(5,tokens[0].line,tokens[0].position);
          return 0;
        }
      for(int i=1;i<tokens.size();i++){
        if(tokens[i].token.length()>16){
          cout<<__parseerror(3,tokens[i].line,tokens[i].position);
          return 0;
        }
        if(!validSymbol(tokens[i].token)){
          cout<<__parseerror(1,tokens[i].line,tokens[i].position);
          return 0;
        }
      }
      if((tokens.size()-1)<stoi(tokens[0].token)){
        cout<<__parseerror(1,endOfFile.line,endOfFile.position);
        return 0;
      }
      if((tokens.size()-1)<stoi(tokens[0].token)){
        cout<<__parseerror(1,endOfFile.line,endOfFile.position);
        return 0;
      }
    }
    else if (partNum==2){
      for(auto& x : symbolTable){
        if(x.second>addressCounter+stoi(tokens[0].token)){
          moduleWarning.push_back("Warning: Module "+to_string(moduleNum)+": "+x.first+" too big "+to_string(x.second)+" (max="+to_string(addressCounter+stoi(tokens[0].token)-1)+") assume zero relative");
          x.second=addressCounter;
        }
      }
      addressCounter+=stoi(tokens[0].token);
      moduleNum++;
      if((tokens.size()-1)/2<stoi(tokens[0].token)){
        if((tokens.size()-1)%2) cout<<__parseerror(0,endOfFile.line,endOfFile.position);
        else cout<<__parseerror(2,endOfFile.line,endOfFile.position);
        return 0;
      }
    } 
    partNum=(partNum+1)%3;
    tokenIndex+=tokens.size();
  }
  int moduleSize=addressCounter;
  for(int x=0;x<moduleWarning.size();x++) 
    cout<<moduleWarning[x]<<endl;
  cout<<"Symbol Table"<<endl;
  for(auto& x : symbolTable)
  {
    if(symbolTableError.find(x.first)!=symbolTableError.end()){
      cout << x.first << "=" << x.second <<" Error: This variable is multiple times defined; first value used"<< endl;
    }
    else cout << x.first << "=" << x.second << endl;
  }
  cout<<endl;
  cout<<"Memory Map"<<endl;

  //second pass
  //clear all varibles except symbol table
  tokenAndPositions.clear();
  tokenAndPositions=tokenizer(argv[1]);//reopen file in tokenizer function
  vector<string> refSym;/*uselist*/ vector<string> UnrefSym;//unusedlist
  addressCounter=0; moduleNum=1; tokenIndex=0;partNum=0;
  while(tokenIndex<tokenAndPositions.size()){
    vector<tokenAndPosition> tokens=modularizer(tokenAndPositions, tokenIndex, partNum, addressCounter);
    if (partNum==1){
      refSym.clear();
      UnrefSym.clear();
      for (int i=1; i<tokens.size();i++){
        refSym.push_back(tokens[i].token);
        UnrefSym.push_back(tokens[i].token);
      } 
    }

    else if (partNum==2){
      int hasRef=0; //record the symbol that has ref
      for (int i=1; i<tokens.size();i+=2){
        int opcode=stoi(tokens[i+1].token)/1000;
        int operand=stoi(tokens[i+1].token)%1000;
        if(opcode>9){
          if(tokens[i].token!="I"){
            cout<<setw(3)<<setfill('0')<<addressCounter+i/2<<": "
          <<9999<<" Error: Illegal opcode; treated as 9999"<<endl; 
          }
          else{
            cout<<setw(3)<<setfill('0')<<addressCounter+i/2<<": "
          <<9999<<" Error: Illegal immediate value; treated as 9999"<<endl; 
          }
          continue;
        }
        if (tokens[i].token=="E"){
          if(operand>=refSym.size()){
            cout<<setw(3)<<setfill('0')<<addressCounter+i/2<<": "
          <<setw(4)<<setfill('0')<<stoi(tokens[i+1].token)<<" Error: External address exceeds length of uselist; treated as immediate"<<endl;
          }
          else if(symbolTable.find(refSym[operand])==symbolTable.end()){
            cout<<setw(3)<<setfill('0')<<addressCounter+i/2<<": "
          <<opcode*1000<<" Error: "<<refSym[operand] <<" is not defined; zero used"<<endl;
          } 
          else{
            cout<<setw(3)<<setfill('0')<<addressCounter+i/2<<": "
            <<opcode*1000+symbolTable[refSym[operand]]<<endl;
            if(symbolUsing.find(refSym[operand])!=symbolUsing.end()){
              symbolUsing.erase(refSym[operand]);
            }
          }
          vector<string>::iterator it=find(UnrefSym.begin(), UnrefSym.end(), refSym[operand]);
          if(it!=UnrefSym.end()) UnrefSym.erase(it);
        }
        else if (tokens[i].token=="A"){
          if(operand>=512){
            cout<<setw(3)<<setfill('0')<<addressCounter+i/2<<": "
          <<opcode*1000<<" "<<"Error: Absolute address exceeds machine size; zero used"<<endl;
          }
          else cout<<setw(3)<<setfill('0')<<addressCounter+i/2<<": "<<setw(4)<<setfill('0')<<stoi(tokens[i+1].token)<<endl;
        }
        else if (tokens[i].token=="I"){
          cout<<setw(3)<<setfill('0')<<addressCounter+i/2<<": "
          <<setw(4)<<setfill('0')<<stoi(tokens[i+1].token)<<endl;
        }
        else if (tokens[i].token=="R"){
          //todo: not sure. module size or total modules size
          //if(operand>=moduleSize)-- total modules size solution
          if (operand>stoi(tokens[0].token)){
            cout<<setw(3)<<setfill('0')<<addressCounter+i/2<<": "
          <<opcode*1000+addressCounter<<" "<<"Error: Relative address exceeds module size; zero used"<<endl;
          }
          else cout<<setw(3)<<setfill('0')<<addressCounter+i/2<<": "<<setw(4)<<setfill('0')<<stoi(tokens[i+1].token)+addressCounter<<endl;
        }
      }
      addressCounter+=stoi(tokens[0].token);
      for(auto &x:UnrefSym)
        cout<<"Warning: Module "+to_string(moduleNum)+": "+x+" appeared in the uselist but was not actually used"<<endl;
      moduleNum++;
    }
    partNum=(partNum+1)%3;
    tokenIndex+=tokens.size();
  }
  cout<<endl;
  for(auto &x:symbolUsing) 
    cout<<"Warning: Module "+to_string(x.second)+": "+x.first+" was defined but never used"<<endl;
  return 0;
}