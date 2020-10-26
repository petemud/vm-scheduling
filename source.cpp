#include <iostream>
#include <vector>
#include <set>
#include <algorithm>
#include <ctime>

struct node {
	int ram, cpu;
} node_config;

struct vm {
	int ram, cpu;
	int nodes;
};

struct location {
	int server_id;
	enum {
		A    = 0b01,
		B    = 0b10,
		BOTH = A | B
	} node_id;
};

struct server {
	node a, b;
};

int new_server(std::vector<server> &servers) {
	server new_server = {
		node_config,
		node_config
	};
	int id = servers.size();
	servers.push_back(new_server);
	return id;
}

int fitness(server srv) {
	return std::min(srv.a.ram + srv.b.ram, srv.a.cpu + srv.b.cpu);
}

location allocate_one_node(vm vm, std::vector<server> &servers) {
	location loc;
	int best_fit = -1;
	for (int id = 0, size = servers.size(); id < size; ++id) {
		auto srv = servers[id];
		int ram_a = srv.a.ram - vm.ram;
		int cpu_a = srv.a.cpu - vm.cpu;
		if (ram_a >= 0 && cpu_a >= 0) {
			int fit = fitness({{ram_a, cpu_a}, srv.b});
			if (best_fit == -1 || fit < best_fit) {
				best_fit = fit;
				loc = {id, location::A};
			}
		}
		int ram_b = srv.b.ram - vm.ram;
		int cpu_b = srv.b.cpu - vm.cpu;
		if (ram_b >= 0 && cpu_b >= 0) {
			int fit = fitness({{ram_b, cpu_b}, srv.a});
			if (best_fit == -1 || fit < best_fit) {
				best_fit = fit;
				loc = {id, location::B};
			}
		}
	}
	if (best_fit == -1)
		loc = {new_server(servers), location::A};
	return loc;
}

location allocate_two_nodes(vm vm, std::vector<server> &servers) {
	location loc;
	loc.node_id = location::BOTH;
	int best_fit = -1;
	for (int id = 0, size = servers.size(); id < size; ++id) {
		auto srv = servers[id];
		int ram_a = srv.a.ram - vm.ram;
		int cpu_a = srv.a.cpu - vm.cpu;
		int ram_b = srv.b.ram - vm.ram;
		int cpu_b = srv.b.cpu - vm.cpu;
		if (ram_a >= 0 && cpu_a >= 0 && ram_b >= 0 && cpu_b >= 0) {
			int fit = fitness({{ram_a, cpu_a}, {ram_b, cpu_b}});
			if (best_fit == -1 || fit < best_fit) {
				best_fit = fit;
				loc.server_id = id;
			}
		}
	}
	if (best_fit == -1)
		loc.server_id = new_server(servers);
	return loc;
}

void allocate(vm vm, node &srv) {
	srv.ram -= vm.ram;
	srv.cpu -= vm.cpu;
}

void free(vm vm, node &srv) {
	srv.ram += vm.ram;
	srv.cpu += vm.cpu;
}

void allocate(vm vm, std::vector<server> &servers, location loc) {
	auto &srv = servers[loc.server_id];
	if (loc.node_id & location::A)
		allocate(vm, srv.a);
	if (loc.node_id & location::B)
		allocate(vm, srv.b);
}

void free(vm vm, std::vector<server> &servers, location loc) {
	auto &srv = servers[loc.server_id];
	if (loc.node_id & location::A)
		free(vm, srv.a);
	if (loc.node_id & location::B)
		free(vm, srv.b);
}

location allocate(vm vm, std::vector<server> &servers) {
	location loc;
	switch (vm.nodes) {
	case 1:
		loc = allocate_one_node(vm, servers);
		break;
	case 2:
		loc = allocate_two_nodes(vm, servers);
	}
	allocate(vm, servers, loc);
	return loc;
}

struct request {
	enum {CREATE, DELETE} type;
	int vm_id;
};

struct answer {
	int max_servers, max_request_id;
	std::vector<location> locs;
};
	
answer start_from_id(int request_id, std::vector<vm> &vms, std::vector<request> &requests) {
	std::set<int> active_vms_set;
	for (int id = 0; id < request_id; ++id) {
		auto req = requests[id];
		switch (req.type) {
		case request::CREATE:
			active_vms_set.insert(active_vms_set.end(), req.vm_id);
			break;
		case request::DELETE:
			active_vms_set.erase(req.vm_id);
			break;
		}
	}
	struct vm_with_id {
		struct vm vm;
		int id;
	};
	std::vector<vm_with_id> active_vms;
	active_vms.reserve(active_vms_set.size());
	for (auto id: active_vms_set) {
		active_vms.push_back({vms[id], id});
	}
	std::random_shuffle(active_vms.begin(), active_vms.end());
	std::sort(active_vms.begin(), active_vms.end(), [](const vm_with_id &a, const vm_with_id &b) {
		return a.vm.ram >= b.vm.ram && a.vm.cpu >= b.vm.cpu && a.vm.nodes >= b.vm.nodes && (a.vm.ram != b.vm.ram || a.vm.cpu != b.vm.cpu || a.vm.nodes != b.vm.nodes);
	});
	std::vector<location> locs(vms.size());
	std::vector<server> servers;
	for (auto vm: active_vms) {
		locs[vm.id] = allocate(vms[vm.id], servers);
	}
	auto copy = servers;
	int max_servers = servers.size(), max_request_id = request_id;
	for (int id = request_id, size = requests.size(); id < size; ++id) {
		auto req = requests[id];
		switch (req.type) {
		case request::CREATE:
			locs[req.vm_id] = allocate(vms[req.vm_id], servers);
			break;
		case request::DELETE:
			if (servers.size() > max_servers) {
				max_servers = servers.size();
				max_request_id = id;
			}
			free(vms[req.vm_id], servers, locs[req.vm_id]);
			break;
		}
	}
	if (servers.size() > max_servers) {
		max_servers = servers.size();
		max_request_id = requests.size();
	}
	servers = copy;
	for (int id = request_id - 1; id >= 0; --id) {
		auto req = requests[id];
		switch (req.type) {
		case request::DELETE:
			locs[req.vm_id] = allocate(vms[req.vm_id], servers);
			break;
		case request::CREATE:
			if (servers.size() > max_servers) {
				max_servers = servers.size();
				max_request_id = id;
			}
			free(vms[req.vm_id], servers, locs[req.vm_id]);
			break;
		}
	}
	if (servers.size() > max_servers) {
		max_servers = servers.size();
		max_request_id = 0;
	}
	return {max_servers, max_request_id, locs};
}

int main() {
	auto program_start = std::clock();
	std::ios::sync_with_stdio(false), std::cin.tie(nullptr);
  
	int requests_number, RAM, CPU;
	std::cin >> requests_number >> RAM >> CPU;
	node_config = {RAM * CPU, CPU * RAM};
	std::vector<vm> vms;
	std::vector<request> requests;
	for (int requests_left = requests_number; requests_left > 0; --requests_left) {
		int request_type;
		std::cin >> request_type;
		switch (request_type) {
		case request::CREATE: {
			int ram, cpu, nodes;
			std::cin >> ram >> cpu >> nodes;
			vm vm = {ram / nodes * CPU, cpu / nodes * RAM, nodes};
			int vm_id = vms.size();
			vms.push_back(vm);
			requests.push_back({request::CREATE, vm_id});
			break;
		}
		case request::DELETE:
			int request_id;
			std::cin >> request_id;
			--request_id;
			requests.push_back({request::DELETE, requests[request_id].vm_id});
			break;
		}
	}
  
	answer best_answer = {-1};
	int start_request = requests_number;
	int sol_id = 1;
	double estimated_time = 4;
	auto sol_start = std::clock();
	auto reading_time = sol_start - program_start;
	do {
		auto new_answer = start_from_id(start_request, vms, requests);
		start_request = new_answer.max_request_id;
		if (best_answer.max_servers == -1 || new_answer.max_servers < best_answer.max_servers) {
			best_answer = new_answer;
		}
		estimated_time = (reading_time + std::clock() - sol_start) / double(CLOCKS_PER_SEC) / (sol_id++) * (sol_id);
	} while (estimated_time < 3.9);
  
	std::cout << best_answer.max_servers << "\n";
	for (auto loc: best_answer.locs) {
		std::cout << loc.server_id + 1;
		switch (loc.node_id) {
		case location::A:
			std::cout << " A";
			break;
		case location::B:
			std::cout << " B";
		}
		std::cout << "\n";
	}
}
