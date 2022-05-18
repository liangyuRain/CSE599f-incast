# CSE599f-incast

## Usage

1. Create `servers.config` file, and add all server deployment information. For example, the following `servers.config` deploys six servers on three physical servers. Each server is listening to `10.0.0.x:x000`. Note: ALWAYS add a new line at the end of `servers.config`.
```
user@server1 10.0.0.1 4000
user@server1 10.0.0.1 5000
user@server2 10.0.0.2 4000
user@server2 10.0.0.2 5000
user@server3 10.0.0.3 4000
user@server3 10.0.0.3 5000

```
2. Run `deploy_server.sh` to deploy the servers. Each server has a tmux session named `server_[PORT]` on the physical server. For example, the first server of the above `servers.config` is going to have a tmux session `server_4000` on `user@server1`.
3. Run `deploy_client.sh [ssh hostname] [--serverCount server_count] [--serverDelay server_delay (us)] [--serverFileSize server_file_size (byte)] [--launchInterval launch_interval (us)] [--numOfExperiments num_of_experiments]` to deploy the client. Only the first argument `[ssh hostname]` (e.g. `user@server4`) is required. `server_count` is 1 by default.
4. Run `kill.sh` to kill all deployed servers.

### Limit Client Bandwidth (Deprecated: configure switch instead)

Run `bash set_bw_limit.sh [ssh hostname] [speed limit (Kbps)]` to limit the downlink speed of target host. A speed limit less than or equal to 0 will clear the speed limit on target host.

## Dell S4080
1. Enter `enable` to enter EXEC Privilege mode.
2. Use `show interfaces | grep "is up"` to see the connected interfaces. Each interface should correspond to one server connected to the switch.
3. Enter `conf` to enter configuration mode.
4. Enter `interface TenGigabitEthernet 1/xx` to enter the configuration mode of specific interface.
5. Use `rate police 1` and `rate shape 1` to limit the incoming speed and outgoing speed to be 1 Mbps.
6. Use `iperf -s` and `iperf -c 10.0.0.x` to find out the speed of which server is limited.