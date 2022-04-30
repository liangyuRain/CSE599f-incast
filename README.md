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