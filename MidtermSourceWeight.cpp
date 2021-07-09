#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>
#include <set>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <random>
#include <iomanip>

using namespace std;

static mt19937 gen = mt19937((unsigned int)time(NULL));
struct node{
    int n_visited;                       //현재 노드를 방문한 횟수
    int n_neighbors;                     //현재 노드의 이웃 숫자
    int inside_neighbors;                //현재 노드가 이웃인 노드 숫자
    int id;                              //현재 노드 id
    int sum_weight;                      //현재 노드의 이웃으로 가는 간선의 가중치 합
    int inside_weight;                   //현재 노드로 들어오는 간선의 가중치 합
    double pageRankVal;                  //현재 노드의 페이지랭크
    string name;                         //현재 노드의 이름
    vector<pair<node *, int>> neighbors; //first : 현재노드의 이웃의 주소를 담고있는 벡터
};                                       //second: 이웃으로 가는 간선의 가중치

class Graph{
public:
    int nodeNum;             //전체 노드 숫자
    vector<node *> nodes;    //전체 노드에 대한 주소를 담고 있는 벡터
    vector<int> id2IdxTable; //노드의 id 와 인덱스가 매핑되어 있는 테이블

    Graph(string filename); //생성자
    ~Graph();
    void LoadEdge(string filename, bool inter);                   // 그래프 생성
    void AddEdge(int source, int target, int weight, bool inter); // 간선 추가
    int Id2Idx(int n);                                          // 노드의 id를 통해 인덱스 반환
    void RandomWalker(int i, int n, float q);                   //i : 시작 노드, n : walking length, q : 랜덤한 노드로 점프할 확률
    void PageRank(int i, int n, float q);                       // i : 시작 노드, n : walking length, q : 랜덤한 노드로 점프할 확률
};

Graph::Graph(string filename){ // 그래프를 생성할 파일 이름을 인자로함
    this->nodeNum = 0;

    std::ifstream ifs(filename);
    std::string line;

    //파일을 라인으로 읽음
    std::getline(ifs, line);
    while (std::getline(ifs, line)){
        // tab로 분할
        std::stringstream ls(line);
        std::string id;
        std::string name;
        std::getline(ls, id, '\t'); //첫 번째 파라 미터를 id로 함
        std::getline(ls, name);     //두 번째 파라미터를 name으로 함

        node *temp = new node;

        //node 소속값 초기화
        temp->n_visited = 0;
        temp->id = std::stoi(id);
        temp->pageRankVal = 0;
        temp->name = name;
        temp->n_neighbors = 0;
        temp->sum_weight = 0;
        temp->inside_weight = 0;
        temp->inside_neighbors = 0;

        nodes.push_back(temp);
        id2IdxTable.push_back(std::stoi(id));   //id를 테이블에 추가
        this->nodeNum++;
    }
    ifs.close();
}

Graph::~Graph(){
    for (vector<node *>::iterator it = nodes.begin(); it != nodes.end(); it++){
        delete (*it);
    }
}

void Graph::AddEdge(int source, int target, int weight, bool inter){ //inter: 노드가 상호간에 가리키는가
    nodes[source]->neighbors.push_back({nodes[target], weight});
    nodes[source]->n_neighbors++;
    nodes[source]->sum_weight += weight;
    nodes[target]->inside_neighbors++;

    if (inter == true){ //서로 가리키는 경우 반대 방향의 간선도 추가
        vector<pair<node *, int>>::iterator it;
        for (it = nodes[target]->neighbors.begin(); it != nodes[target]->neighbors.end(); it++){
            if ((*it).first == nodes[source])
                break;
        }
        if (it == nodes[target]->neighbors.end()){
            nodes[target]->neighbors.push_back({nodes[source], weight});
            nodes[target]->n_neighbors++;
            nodes[target]->sum_weight += weight;
            nodes[source]->inside_neighbors++;
        }
    }
}

int Graph::Id2Idx(int n){    // 노드의 id를 통해 인덱스 반환
    auto it = find(id2IdxTable.begin(), id2IdxTable.end(), n);
    if (it == id2IdxTable.end()){
        cout << "node not found\n";
        return -1;
    }
    else{
        return std::distance(id2IdxTable.begin(), it);
    }
}

void Graph::LoadEdge(string filename, bool inter){//inter: 노드가 상호간에 가리키는가
    std::ifstream ifs(filename);
    std::string line;

    //파일을 라인으로 읽음
    std::getline(ifs, line);
    while (std::getline(ifs, line)){
        // tab로 분할
        std::stringstream ls(line);
        std::string source;
        std::string target;
        std::string weight;
        std::getline(ls, source, '\t'); //첫 번째 파라미터를 source으로 함
        std::getline(ls, target, '\t'); //두 번째 파라미터를 target으로 함
        std::getline(ls, weight);       //세 번째 파라미터를 가중치로 함

        int idxS = Id2Idx(std::stoi(source)); // Vector index of node source
        int idxT = Id2Idx(std::stoi(target)); // Vector index of node target
        int valW = std::stoi(weight);

        AddEdge(idxS, idxT, valW, inter);
    }
    ifs.close();

    /*for (vector<node *>::iterator it = nodes.begin(); it != nodes.end(); it++){
        cout << "id : " << (*it)->id << endl             << "neighbors : ";
        for (vector<pair<node *, int>>::iterator tt = (*it)->neighbors.begin(); tt != (*it)->neighbors.end(); tt++)
            cout << (*tt).first->id << " ";
        cout << endl             << endl;
    }*/
}

void Graph::RandomWalker(int i, int n, float q){ // i : 시작 노드, n : walking length, q : 랜덤한 노드로 점프할 확률
    int idxNode = i;
    int jumpRand;//랜덤으로 이동할 노드의 인덱스
    int jumpRandId;//랜덤으로 이동할 노드의 id
    int jumpNeighborRand;//이웃한 노드 중 랜덤으로 이동할 노드의 이웃 인덱스
    int jumpNeighborId;//이웃한 노드 중 랜덤으로 이동할 노드 id
    static uniform_int_distribution<int> distJumpORNeighbor(1,100);

    nodes[idxNode]->n_visited++; //처음 시작 위치 노드의 방문 횟수 증가

    for(int k=0;k<n-1;k++){//총 n-1번 반복, 처음 시작 노드가 증가하고 시작하기 때문
        int jumpOrNeighbor = distJumpORNeighbor(gen); //이웃하지 않는 노드로 이동할 확률

        if(nodes[idxNode]->n_neighbors == 0)
            jumpOrNeighbor = 1;//현재 노드에 이웃이 없는 경우 무조건 랜덤하게 이동하도록 함
        
        if(jumpOrNeighbor <= (int)(100*q) && jumpOrNeighbor >= 1){//이웃하지 않는 노드 탐색
            uniform_int_distribution<int> distJumpRand(0,nodeNum-1); //random의 범위를 0<= n < nodeNum
            jumpRand = distJumpRand(gen); //랜덤으로 이동할 노드의 인덱스
            jumpRandId = nodes[jumpRand]->id; //랜덤으로 이동할 노드의 id
            nodes[jumpRand]->n_visited++; //랜덤으로 이동한 노드의 방문 횟수 증가
            idxNode = jumpRand; //다음 반복문의 시작위치를 랜덤으로 이동한 노드로 함
        }
        else{
            uniform_int_distribution<int> distNeighborRand(0,nodes[idxNode]->sum_weight-1);//random의 범위를 0<= n < sum_weight
            int sumNeighborWeight = distNeighborRand(gen); //현재 선택한 이웃으로 이동할 지 판별하는 변수
            vector<pair<node *, int>>::iterator it;
            for (it = nodes[idxNode]->neighbors.begin(); it != nodes[idxNode]->neighbors.end(); it++){//이웃 반복문으로 탐색
                sumNeighborWeight -= (*it).second; //생성한 난수에 선택된 이웃으로의 가중치를 뺌
                if(sumNeighborWeight < 0){ //만약 0보다 작아진 다면 선택한 이웃으로 이동
                    jumpNeighborRand = Id2Idx((*it).first->id); //이웃으로 이동할 노드의 인덱스
                    jumpNeighborId = nodes[jumpNeighborRand]->id; //이웃으로 이동할 노드의 id
                    nodes[jumpNeighborRand]->n_visited++; //이웃으로 이동한 노드의 방문 횟수 증가
                    idxNode = jumpNeighborRand; //다음 반복문의 시작위치를 이웃 중 이동한 노드로 함
                    break;
                }
            }
        }
    }
}

void Graph::PageRank(int i, int n, float q){
    vector<pair<double,int>> sortedNode; //전체 노드에 대해 first : pageLank, second : id 로하는 벡터 생성
    int sum_visited = 0;
    double sum_pagerank = 0;

    RandomWalker(i,n,q); //랜덤 워크 수행

    for (vector<node *>::iterator it = nodes.begin(); it != nodes.end(); it++){ //전체 노드에 대해
        (*it)->pageRankVal = (double) (*it)->n_visited/n; // 주어진 식으로 pageLank 수정
        sortedNode.push_back({(*it)->pageRankVal, (*it)->id}); //벡터 페어에 대입
        sum_visited += (*it)->n_visited;
        sum_pagerank += (*it)->pageRankVal;
        vector<pair<node *, int>>::iterator iter;
        for (iter = (*it)->neighbors.begin(); iter != (*it)->neighbors.end(); iter++){//이웃 반복문으로 탐색
            (*iter).first->inside_weight += (*iter).second;//이웃에 대해 이웃으로 가는 가중치 값 증가
        }
    }
    sort(sortedNode.begin(), sortedNode.end()); //first 값인 pageLank로 오름차순 정렬

    int k = nodeNum;
    for (vector<pair<double,int>>::iterator it = sortedNode.begin(); it != sortedNode.end(); it++){
        cout << left << "rank : " << setw(4)  << k 
                    <<"    id : " << setw(4) << it->second  
                    <<"    inside_neighbors : " << setw(4) << this->nodes[Id2Idx(it->second)]->inside_neighbors  
                    << "    pagerank : "  << setw(13) << it->first 
                    << "    inside_weight : " << setw(4) << this->nodes[Id2Idx(it->second)]->inside_weight 
                    << "  name : " << this->nodes[Id2Idx(it->second)]->name << endl;
        k--;
    }//정렬된 정보 출력

    cout << "sumOfPagerank : " << sum_pagerank <<  "    sumOfVisited: " << sum_visited  << "    sumOfNodes: " << nodeNum << endl;
}
int main(){
    Graph StarW_All = Graph("./dataset/starwars/starwars-full-interactions-allCharacters-nodes.tsv");
    Graph BicycleT_All = Graph("./dataset/bicycle/station_names.tsv");

    StarW_All.LoadEdge("./dataset/starwars/starwars-full-interactions-allCharacters-links.tsv", true);
    BicycleT_All.LoadEdge("./dataset/bicycle/bicycle_trips_all.tsv", false);

    int i = 0;
    int n = 3000000;
    float q = 0.5;
    //StarW_All.PageRank(i,n,q);
    BicycleT_All.PageRank(i,n,q);
}