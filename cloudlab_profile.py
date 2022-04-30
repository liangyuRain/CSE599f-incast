""" Starfish topology with uniform BW, and Tunable number of workers. Master is a separate VM.
All machine has Ubuntu_16_04_java image
Master IP 10.0.0.253
Host IP 10.0.0.[i]

v1.1 Bug fix
v1.2 Change master name to dc3master
Instructions:
"""
import geni.rspec.emulab
import geni.portal
import geni.rspec.pg
import geni.urn

# Describe the parameter(s) this profile script can accept.
# geni.portal.context.defineParameter( "NUM_DC", "number of datacenters", geni.portal.ParameterType.INTEGER, 2)

geni.portal.context.defineParameter("NUM_WORKERS", "number of total workers", geni.portal.ParameterType.INTEGER, 5)
geni.portal.context.defineParameter("INTRA_BW", "intra datacenter bandwidth in kbps", geni.portal.ParameterType.INTEGER,
                                    1000000)

geni.portal.context.defineParameter("size_mnt", "The size for /mnt (GB)", geni.portal.ParameterType.INTEGER, 80)
geni.portal.context.defineParameter("size_home", "The size for /users (GB)", geni.portal.ParameterType.INTEGER, 15)

# Retrieve the values the user specifies during instantiation.
params = geni.portal.context.bindParameters()

NUM_WORKERS = params.NUM_WORKERS
# NUM_DC = NUM_WORKERS

INTRA_BW = params.INTRA_BW

size_mnt = params.size_mnt
size_home = params.size_home

NET_MASK = "255.255.255.0"

# Create a Request object to start building the RSpec.
request = geni.portal.context.makeRequestRSpec()

# Create topology
# TODO create the topology.

#  BW is enforced on interface, so for each worker, create interface.
#  then create a single lan, and add all interface to it.

hosts = [None for _ in range(NUM_WORKERS)]


# lan
lan = request.LAN("lan")
# lan.link_multiplexing = True


# Workers
for i in range(NUM_WORKERS):
    host_name = "h" + str(i + 1)
    hosts[i] = request.RawPC(host_name)
    hosts[i].disk_image = geni.urn.Image("emulab.net", "emulab-ops:UBUNTU20-64-STD")
    bs_mnt = hosts[i].Blockstore(host_name, "/mnt")
    bs_mnt.size = str(size_mnt) + 'GB'
    bs_home = hosts[i].Blockstore(host_name + '1', "/users")
    bs_home.size = str(size_home) + 'GB'

    hosts[i].hardware_type = "c6420"

    # interface and LAN
    ip_worker = geni.rspec.pg.IPv4Address("10.0.0." + str(i + 1), NET_MASK)
    if_worker = hosts[i].addInterface("if" + str(i + 1), ip_worker)
    if_worker.bandwidth = INTRA_BW
    lan.addInterface(if_worker)



# Print the RSpec to the enclosing page.
geni.portal.context.printRequestRSpec()