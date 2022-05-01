base_ip_suffix = 2
base_port = 3000
num_per_server = 5

if __name__ == "__main__":
    nums = [5, 8, 2, 15, 6, 17, 20, 26]
    with open("servers.config", "w") as f:
        for i in range(len(nums)):
            for j in range(num_per_server):
                f.write(f"liangyu@apt{nums[i]:03}.apt.emulab.net 10.0.0.{2 + i} {base_port + j}\n")
