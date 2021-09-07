// #include <iostream>
// #include <vector>
// #include <fstream>
// #include <string>
// #include <sstream>
// #include <string>
// #include <utility>
// #include <set>
// #include <limits>
// #include <queue>


// int maxNode = 0;
// vector<vector<int>> adjMatrix;
// vector<tuple<int,int>> msg;
// vector<string> _msg;
// ofstream out;


// int getSize(string topoFile);
// vector<tuple<int,int>> get_msg(string msg_file);
// vector<string> get_msg_(string msg_file);
// void printMSG( vector<int<vector>> predessors )


// //topology -> message -> change -> message ->change
// int main(int argc, char** argv) {
//     //printf("Number of arguments: %d", argc);
//     if (argc != 4) {
//         printf("Usage: ./distvec topofile messagefile changesfile\n");
//         return -1;
//     }
//     maxNode = getSize(argv[0]);
//     adjMatrix = readTopo(argv[0]);
//     msg = get_msg(argv[1]);
//     _msg = get_msg_(argv[1]);
//     //print initial forwarding table and message 
//     vector<int<vector>> predessors(maxNode);

//     for(int i = 0; i< maxNode; i++){
//         predessors[i] = bellmanFord(i);
//     }
//     out.open("output.txt");
//     //print forwarding table + msg
//     printMSG(predessors);
//     //print all change
//     ifstream change;
//     change.open(argv[3]);
//     std::string line;
//     int src,dest,cost;
//     while (std::getline(change, line))
//     {
//         std::istringstream iss(line);
//         if (!(iss >> src >> dest >> cost)) { break; } // error
//         adjMatrix[sec][dest] = cost;
//         adjMatrix[dest][src] = cost;
//         // re-apply bellman ford on all nodes
//         for(int i = 0; i< maxNode; i++){
//              predessors[i] = bellmanFord(i);
//         }
//         //print forwarding table and msg
//         printMSG(predessors);
//     }
//     out.close();
//     return 0;
// }
// /* 
// * print updated forwarding graph and all msg path taken to file 
//  */
// void printMSG( vector<int<vector>> predessors ){
//     //print forwarding table 
//     for(int i = 0; i< maxNode; i++ ){
//         int next = 0 ;
//         for(int j = 0; j < maxNode; j++){
//             if(predessors[i][j] = i ){
//                 next = j;
//                 break;
//             }
//         }
//         for(int k = 0 ; k < maxNode; k++){
//             out << i << " " << next << " " << predessors[i][k]<< endl;
//         }
//     }
//     // print all msg
//     vector<int> to_print;
//     for(int itr = 0; itr!= maxNode; itr ++){
//         int src = msg[itr][0];
//         int dest = msg[itr][1];
//         out << "from " << src << " to " << dest << " hops";
//         while(dest != INT_MAX){ // REVIEW: will print sec itself?
//             to_print.push_back(predessors[src][dest]);
//         }
//         for (vector<my_class>::reverse_iterator p = to_print.rbegin(); 
//         p != to_print.rend(); ++p ) { 
//             out << " " << to_print[p];
//         }   
//          out << " message" << _msg[itr] << endl;    
//     }
// }



// /* 
// *   readTopo: read initial topology, setup data structure
//  */
// vector<vector<int>> readTopo(string topo,int maxNode){
//     vector<vector<int>> ret(maxNode);
//     for(int i = 0; i < maxNode; ++i ){
//         ret[i] = vector<int>(maxNode);
//         for(int j = 0; j < maxNode; ++j){
//             if(i ==j) {
//                 ret[i][j] = 0}
//         }else{
//             ret[i][j] = INT_MAX;
//         }
//     }
//     // read from file 
//     std::string line;
//     int start,end,cost;
//     ifstream File;
//     File.open(topo);
//     while (std::getline(topo, line))
//     {
//         std::istringstream iss(line);
//         if (!(iss >> start >> end >> cost)) { break; } // error
//         adjMatrix[start][end] = cost;
//         adjMatrix[end][start] = cost;
//     }
//     return ret;
// }   


// int getSize(string topoFile) {
//     string line;
//     ifstream File;
//     File.open(topoFile);
//     int maxNode = -1;
//     while (getline(File, line))
//     {
//         istringstream iss(line);
//         int start, end, cost;
//         if (!(iss >> start >> end >> cost)) { break; } // error
//         maxNode = max(maxNode, start);
//         maxNode = max(maxNode, end);
//     }
//     return maxNode;
// }


// /* 
// * return shortest path from src to every node
// * ret: predessor vector
//  */
// vector<int> bellmanFord(node src){
//     vector<int> predessor(maxNode)= {};
//     predessor[src] = INT_MAX;
//     for(int i = 0 ; i < maxNode ; i++){
//         for(int j = 0; j < maxNode; j++  ){
//             for(int k = 0; j< maxNode; k++){
//                 if(adjMatrix[j][k] == INT_MAX) continue;
//                     int curr_cost = adjMatrix[src][k];
//                     int _cost = adjMatrix[src][j] + adjMatrix[j][k];
//                     if(_cost < curr_cost) {
//                         adjMatrix[src][k] = _cost;
//                         predessor[k] = j;
//                     }else if(_cost == curr_cost){
//                         predessor[k]  =  j < k ? j:K; // tie break
//                     }
//             }
//         }
//     }
//     // curr = dest
//     // while(int curr != INT_MAX){
//     //     ret.push_back(curr);
//     //     curr = predessor[curr];
//     // }
//     return predessor;
// }

// vector<string> get_msg_(string msg_file){
//     vector<string> ret;
//     std::string line;
//     int src,dest;
//     string msg;
//     ifstream File;
//     File.open(topo);
//     while (std::getline(msg_file, line))
//     {
//         std::istringstream iss(line);
//         int a, b;
//         if (!(iss >> src >> dest >> msg)) { break; } // error
//         ret.push_back(msg);
//     }
//     return ret;
// }

// vector<tuple<int,int>> get_msg(string msg_file){
//     vector<tuple<int,int>> ret;
//     std::string line;
//     int src,dest;
//     string msg;
//     ifstream File;
//     File.open(topo);
//     while (std::getline(msg_file, line))
//     {
//         std::istringstream iss(line);
//         int a, b;
//         if (!(iss >> src >> dest >> msg)) { break; } // error
//         ret.push_back(<sec,dest>);
//     }
//     return ret;
// }

#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <sstream>
#include <string>
#include <utility>
#include <set>
#include <limits>
#include <queue>
#include <stack> 
using namespace std;

int getSize(string topoFile);
vector<pair<int, int> > getDistanceVector(vector<vector<int> > adjMatrix, int maxNode, int startIndex);
void printForwardTable(vector<vector<int> > adjMatrix, int maxNode);
void printMessages(vector<vector<int> > adjMatrix, string messageFile, int maxNode);
ofstream output;

int main(int argc, char* argv[]) {
    if (argc != 4) {
        cout << "Usage: ./linkstate topofile messagefile changesfile" << endl;
        return -1;
    }
    output.open("output.txt");
    vector<string> allArgs(argv, argv + argc);
    string topoFile = allArgs[1];
    string messageFile = allArgs[2];
    string changesFile = allArgs[3];
    int maxNode = getSize(topoFile);
    // initializing adjacency matrix
    vector<vector<int> > adjMatrix(maxNode, vector<int> (maxNode, 0));

    for (int i = 0; i < maxNode; i++) {
        // adjMatrix[i] = vector<int>(maxNode);
        for (int j = 0; j < maxNode; j++) {
            adjMatrix[i][j] = -1;
            if (i == j) {
                // cost is 0
                adjMatrix[i][j] = 0;
            }
        }
    }
    // building adjacency matrix
    string line;
    ifstream File;
    File.open(topoFile);
    while (getline(File, line))
    {
        istringstream iss(line);
        int start, end, cost;
        if (!(iss >> start >> end >> cost)) { break; } // error
        // matrix is 0-indexed
        start--;
        end--;
        // undirected graph
        adjMatrix[start][end] = cost;
        adjMatrix[end][start] = cost;
    }

    // printing forward table (first time)
    printForwardTable(adjMatrix, maxNode);

    // dealing with messages (first time)
    printMessages(adjMatrix, messageFile, maxNode);

    // apply changes
    File.close();
    File.open(changesFile);
    while (getline(File, line))
    {
        istringstream iss(line);
        int start, end, cost;
        if (!(iss >> start >> end >> cost)) { break; } // error
        // modifying edges in the adjacenty matrix
        start--;
        end--;
        if (cost == -999) {
            // remove edge
            adjMatrix[start][end] = -1;
            adjMatrix[end][start] = -1;
        } else {
            adjMatrix[start][end] = cost;
            adjMatrix[end][start] = cost;
        } 
        // reprinting fowardtable and messages
        printForwardTable(adjMatrix, maxNode);
        printMessages(adjMatrix, messageFile, maxNode);

    }
    return 0;
}

int getSize(string topoFile) {
    string line;
    ifstream File;
    File.open(topoFile);
    int maxNode = -1;
    while (getline(File, line))
    {
        istringstream iss(line);
        int start, end, cost;
        if (!(iss >> start >> end >> cost)) { break; } // error
        maxNode = max(maxNode, start);
        maxNode = max(maxNode, end);
    }
    return maxNode;
}

vector<pair<int, int> > getDistanceVector(vector<vector<int> > adjMatrix, int maxNode, int startIndex) {
    set<int> visited;
    vector<pair<int, int> > dstVector(maxNode);
    // first means the last previous node, second means the total cost
    for (int i = 0; i < maxNode; i++) {
        // inifinity
        dstVector[i].first = -1;
        dstVector[i].second = numeric_limits<int>::max();
        // cost is 0
        if (i == startIndex) {
            dstVector[i].first = i;
            dstVector[i].second = 0;
        } else if (adjMatrix[startIndex][i] > -1) {
            dstVector[i].first = startIndex;
            dstVector[i].second = adjMatrix[startIndex][i];
        }
    }
    
    visited.insert(startIndex);
    while (visited.size() < maxNode) {
        // find min dst index
        int minDstIndex = -1;
        int minDst = numeric_limits<int>::max();
        for (int i = 0; i < maxNode; i++) {
            if (visited.find(i) == visited.end() && dstVector[i].second <= minDst) {
                minDst = dstVector[i].second;
                minDstIndex = i;
            }
        }
        if (minDst == numeric_limits<int>::max()) {
            // reamaining nodes are all unreachable
            break;
        }
        int currentIndex = minDstIndex;
        visited.insert(currentIndex);
        for (int i = 0; i < maxNode; i++) {
            if (visited.find(i) == visited.end() && adjMatrix[currentIndex][i] > -1) {
                // is neighbor and not visited, update dst if possible
                if (dstVector[i].second > dstVector[currentIndex].second + adjMatrix[currentIndex][i] || (dstVector[i].second == (dstVector[currentIndex].second + adjMatrix[currentIndex][i]) && currentIndex < dstVector[i].first)) {
                    // handles tie breaking as well
                    dstVector[i].first = currentIndex;
                    dstVector[i].second = dstVector[currentIndex].second + adjMatrix[currentIndex][i];
                } 
            }
        }
    }
    return dstVector;
    
}

void printForwardTable(vector<vector<int> > adjMatrix, int maxNode) {
    for (int i = 0; i < maxNode; i++) {
        vector<pair<int, int> > dstVector = getDistanceVector(adjMatrix, maxNode, i);
        for (int end = 0; end < dstVector.size(); end++) {
            if (dstVector[end].second == numeric_limits<int>::max()) {
                // skip unreachable entries
                continue;
            }
            int prev = end;
            int current = dstVector[end].first;
            int nextHop = -1;
            while (current != i) {
                prev = current;
                current = dstVector[current].first;
            }
            nextHop = prev;
            output << end + 1 << " " << nextHop + 1 << " " << dstVector[end].second << endl;
        }
    }
}

void printMessages(vector<vector<int> > adjMatrix, string messageFile, int maxNode) {
    // read messages
    string line;
    ifstream File;
    File.open(messageFile);
    while (getline(File, line))
    {
        string temp;
        istringstream iss(line);
        int start, end;
        string message;
        iss >> start;
        getline(iss, temp, ' ');
        iss >> end;
        getline(iss, temp, ' ');
        getline(iss, message, '\n');

        start--;
        end--;
        // dealing with one message
        vector<pair<int, int> > dstVector = getDistanceVector(adjMatrix, maxNode, start);
        if (dstVector[end].second == numeric_limits<int>::max()) {
            // unreachable
            output << "from " << start + 1 << " to " << end + 1 << " cost infinite hops unreachable " << "message " << message << endl;
            continue;
        }
        stack<int> path;
        int prev = end;
        path.push(prev);
        int current = dstVector[end].first;
        path.push(current);
        while (current != start) {
            prev = current;
            current = dstVector[current].first;
            path.push(current);
        }
        output << "from " << start + 1 << " to " << end + 1 << " cost " << dstVector[end].second << " hops ";
        while(!path.empty()) {
            int current = path.top();
            path.pop();
            if (!path.empty()) {
                output << current + 1 << " ";
            }
        }
        output << "message " << message << endl;

    }
    
}
