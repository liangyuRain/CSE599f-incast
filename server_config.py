server_nums = [] # server nums
client_server = None # client server num

base_ip_suffix = 1
base_port = 3000
num_per_server = 100

if __name__ == "__main__":
    with open("servers.config", "w") as f:
        for j in range(num_per_server):
            for i in range(len(server_nums)):
                if server_nums[i] == client_server:
                    continue
                f.write(f"liangyu@hp{server_nums[i]:03}.utah.cloudlab.us 10.0.0.{base_ip_suffix + i} {base_port + j}\n")
