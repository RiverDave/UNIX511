/******************************************************************************
* UNX511-Lab1
* I declare that this lab is my own work in accordance with Seneca Academic Policy.
* No part of this assignment has been copied manually or electronically from any other source
* (including web sites) or distributed to other students.
*
* Name: David Rivera Student ID: 137648226 Date: 17/01/25
*
*
******************************************************************************/



#include "pidUtil.h"
#include "stdio.h"
#include <vector>
#include <string>
#include <cassert>
#include <optional>
#include <limits.h>


void print_pids(const std::vector<int>& pids){

	for(auto &pid : pids)
		std::cout <<  pid << "\n";
}

int main(void) {

  using namespace std;

  vector<int> pidList;

  {

  // 1.
  std::cout << "====== 1. Getting and printin' all PIDS ======\n";
  ErrStatus err = GetAllPids(pidList);
  if(err) cout << GetErrorMsg(err) << "\n";
  else{
  	assert(!pidList.empty());
  	print_pids(pidList);
  }




  }



  {

  // 2.
  std::cout << "====== 2. Printing name of PID 1 ======\n";
  const int pid_one= 1;
  std::string pid_name;
  ErrStatus err = GetNameByPid(pid_one, pid_name);
  if(err) cout << GetErrorMsg(err) << "\n";
  else{
  	assert(!pid_name.empty());
  	std::cout << pid_name << std::endl;
  }



  }



  {
  // 3.
  const std::string name = "Lab2";
  std::cout << "====== 3. Printing PID with name: " << name <<  " ======\n";
  int pid_id = INT_MAX; // we use max as placeholder here
  ErrStatus err = GetPidByName(name, pid_id);
  if(err) cout << GetErrorMsg(err) << "\n";
  else{

  	assert(pid_id != INT_MAX);
  	std::cout << pid_id << endl;

  }


  }




  {
  // 4.
  const std::string name = "Lab11";
  std::cout << "====== 4. Printing PID with name: " << name <<  " ======\n";
  int pid_id = INT_MAX; // we use max as placeholder here
  ErrStatus err = GetPidByName(name, pid_id);
  if(err) cout << "ERR: " <<  GetErrorMsg(err) << "\n";
  else{

  	assert(pid_id != INT_MAX);
  	std::cout << pid_id << endl;

  }
  }




  return 0;
}
