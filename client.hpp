#include <iostream>
#include <vector>
#include <queue>

using namespace std;

using ll = long long;

const ll INF = -1e18; 

long long HalfDiscount(int N, int M, const vector<vector<int>>& edges) {
    int num_nodes = 2 * N;
    vector<vector<pair<int, int>>> adj(num_nodes + 1);

    for (const auto& edge : edges) {
        int u = edge[0];
        int v = edge[1];
        int cost = edge[2];

        adj[u].push_back({v, cost});

        adj[u + N].push_back({v + N, cost});
        
        adj[u].push_back({v + N, cost / 2});
    }

    vector<ll> dist(num_nodes + 1, INF);
    vector<int> count(num_nodes + 1, 0);
    vector<bool> inQueue(num_nodes + 1, false);
    queue<int> q;

    int start_node = 1;
    dist[start_node] = 0;
    q.push(start_node);
    inQueue[start_node] = true;

    while (!q.empty()) {
        int u = q.front();
        q.pop();
        inQueue[u] = false;

        for (const auto& edge : adj[u]) {
            int v = edge.first;
            int weight = edge.second;

            if (dist[u] != INF && dist[u] + weight > dist[v]) {
                dist[v] = dist[u] + weight;

                if (!inQueue[v]) {
                    q.push(v);
                    inQueue[v] = true;
                    count[v]++;

                    if (count[v] > num_nodes) {
                        return -1;
                    }
                }
            }
        }
    }
    return dist[N + N];
}