#include <stdlib.h>
#include <iostream>
#include <string>
#include <fstream>
#include <queue>
#include <sstream>
#include <vector>
#include <climits>
#include <algorithm>
#include <math.h>

using namespace std;

class Node
{
public:
  int index;
  int backoff;
  int rand_range;
  int retry;
  void setRandom(int R);
};

void Node::setRandom(int R)
{
// can pick 0
  this->backoff = rand() % (R + 1);
}

vector<Node>nodes;
vector<int> collision_per_node;
vector<int> success_per_node;
long total_collision_cnt;
long total_success_cnt;
long channel_idle;
long channel_used;
long packet_send;
int N,L,M,T; // N: node count ; L: pkt_size ; M: Maximum transmission attempt ; T: time of simulation
vector<int>R; // R: Rand range


int findVar(vector<int> arr,int is_collision){
    int var = 0;
    int mean =0;
    mean = is_collision ? total_collision_cnt/N : total_success_cnt/N;
    for( int n = 0; n < N; n++ )    
    {
        var += ((arr[n] - mean) * (arr[n] - mean));
    }
    var /= N;
    return var;
}

// write output file 
void output() {

	ofstream outfile;
	outfile.open("output.txt");
	outfile << "Channel utilization (in percentage) " << ((double)channel_used / T) * 100.0 << endl;
	outfile << "Channel idle fraction (in percentage) " << ((double)channel_idle / T) * 100.0 << endl;
	outfile << "Total number of collisions " << total_collision_cnt << endl;
	outfile << "Variance in number of successful transmissions (across all nodes) " << findVar(success_per_node,0) << endl;
	outfile << "Variance in number of collisions (across all nodes) " << findVar(collision_per_node,1) << endl;
	outfile.close();

}

//prepare global variables
void init(){
    total_collision_cnt =  0;
    total_success_cnt = 0;
    channel_idle = 0;
    channel_used = 0;
    packet_send = 0 ;
    srand (time(NULL));
}

// prepare node data structure 
void makeNodes(){
    int i;
    Node mynode;
    for(i = 0; i < N ; i++){
        mynode.index = i;
        mynode.backoff = 0;
        mynode.rand_range = R[mynode.backoff];
        mynode.retry = 0;
        nodes.push_back(mynode);
        collision_per_node.push_back(0);
        success_per_node.push_back(0);
    }
}

// set every node an initial random backoff 
void initNodes(){
    for(int i =0 ;i < N; i++){
        nodes[i].setRandom(nodes[i].rand_range);
        // cout << "node "<< i << "init set to "<< nodes[i].backoff<<endl;
    }
}

// get all nodes which are ready to send pkt at this point
vector<int> getReadyNode(){
    vector<int> ret;
    for(int i =0 ;i < N; i++){
        if(nodes[i].backoff == 0 ) { 
            ret.push_back(nodes[i].index);
            // cout << "node "<< i << " is free" << endl;
        }
    }
    return ret;
}

void collision(Node mynode){
    mynode.retry ++;
    if(mynode.retry == M - 1){
        // drop packet, reset R, collision, reset backoff
        mynode.retry = 0;
        mynode.rand_range = R[mynode.retry];
        mynode.setRandom(mynode.rand_range);
    }else{
        // double rand_range
        mynode.rand_range  = R[mynode.retry];
    }   
}

void simulate(){
    int time = 0;
    makeNodes();
    initNodes();
    int curr_pkt = 0; //remaining of current transmission
    int curr_transmit = -1; // node which is currently transmissting 
    while(time < T){
        if(curr_pkt > 0){
        // channel busy, freeze all count down and continue transmit
        channel_used ++;
        curr_pkt --;
        }else{
            //re-rand previously transmiited node 
            if(curr_transmit != -1){
                total_success_cnt ++;
                success_per_node[curr_transmit] ++;
                nodes[curr_transmit].setRandom(nodes[curr_transmit].rand_range);
                // cout << "node " << curr_transmit << "done transmission; rerand to "<< nodes[curr_transmit].backoff<< endl;
                curr_transmit = -1;
            }
            // channel idle, find new node to transmit
            vector<int> rdy = getReadyNode();
            if(rdy.size() == 0){
                // no ready node
                channel_idle ++;
                for (int i = 0 ;i < N; i++){
                    if(nodes[i].backoff) nodes[i].backoff --;
                }
            }else{
                // this cycle counted as used 
                channel_used ++;
                // pick a random node from list
                int lucky;
                lucky = rand() % (rdy.size());
                // cout << "pick node "<<rdy[lucky]<<endl;
                curr_pkt = L;
                curr_transmit = rdy[lucky];
                for(int i : rdy){
                    if(nodes[i].index != lucky){
                        // fail to be picked, collision 
                        collision(nodes[i]);
                        collision_per_node[i]++;
                        total_collision_cnt ++;
                    }
                }
            }
        }
        time ++;
    }   
    output();
}

// partially from  https://github.com/f1mm/ece438/blob/master/mp4/src/csma.cpp
int main(int argc, char** argv){

    if (argc != 2)
	{
		fprintf(stderr, "usage: ./csma input.txt\n");
		exit(1);
	}

	char temp;

    // parse input

    ifstream input_(argv[1]);
    input_ >> temp >> N >> temp >> L >> temp;
    // cout << "N: "<< N << " L: "<< L << " R:";

    while (input_.get() != 10) { // while we are not at end of line
	    int backoff_time;
		input_ >> backoff_time;
        // cout << backoff_time << " ";
		R.push_back(backoff_time);
	}
    // cout << endl;
    input_ >> temp >> M >> temp >> T;
    // cout << "M: "<< M << " T: "<< T << endl;
    init();
    simulate();
}






