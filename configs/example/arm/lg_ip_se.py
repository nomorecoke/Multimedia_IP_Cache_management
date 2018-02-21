# Copyright (c) 2016-2017 ARM Limited
# All rights reserved.
#
# The license below extends only to copyright in the software and shall
# not be construed as granting a license to any other intellectual
# property including but not limited to intellectual property relating
# to a hardware implementation of the functionality of the software
# licensed hereunder.  You may use the software subject to the license
# terms below provided that you ensure that this notice is replicated
# unmodified and in its entirety in all distributions of the software,
# modified or unmodified, in source code or in binary form.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met: redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer;
# redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution;
# neither the name of the copyright holders nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
#  Authors:  Andreas Sandberg
#            Chuan Zhu
#            Gabor Dozsa
#

"""This script is the syscall emulation example script from the ARM
Research Starter Kit on System Modeling. More information can be found
at: http://www.arm.com/ResearchEnablement/SystemModeling
"""

import os
import m5
from m5.util import addToPath
from m5.objects import *
import argparse
import shlex
import spec06_benchmarks
import ConfigParser
import optparse
import sys

m5.util.addToPath('../..')

from common import MemConfig
from common.cores.arm import HPI
from common.Caches import *

import devices


# Pre-defined CPU configurations. Each tuple must be ordered as : (cpu_class,
# l1_icache_class, l1_dcache_class, walk_cache_class, l2_Cache_class). Any of
# the cache class may be 'None' if the particular cache is not present.
cpu_types = {
    "atomic" : ( AtomicSimpleCPU, None, None, None, None),
    "minor" : (MinorCPU,
               devices.L1I, devices.L1D,
               devices.WalkCache,
               devices.L2),
    "hpi" : ( HPI.HPI,
              HPI.HPI_ICache, HPI.HPI_DCache,
              HPI.HPI_WalkCache,
              HPI.HPI_L2)
}


class SimpleSeSystem(System):
    '''
    Example system class for syscall emulation mode
    '''

    # Use a fixed cache line size of 64 bytes
    cache_line_size = 64

    def __init__(self, args, **kwargs):
        super(SimpleSeSystem, self).__init__(**kwargs)

        # Setup book keeping to be able to use CpuClusters from the
        # devices module.
        self._clusters = []
        self._num_cpus = 0

        # Create a voltage and clock domain for system components
        self.voltage_domain = VoltageDomain(voltage="3.3V")
        self.clk_domain = SrcClockDomain(clock="1GHz",
                                         voltage_domain=self.voltage_domain)

        # Create the off-chip memory bus.
        self.membus = SystemXBar()

        # Wire up the system port that gem5 uses to load the kernel
        # and to perform debug accesses.
        self.system_port = self.membus.slave

        # Add CPUs to the system. A cluster of CPUs typically have
        # private L1 caches and a shared L2 cache.
        self.cpu_cluster = devices.CpuCluster(self,
                                              args.num_cores,
                                              args.cpu_freq, "1.2V",
                                              *cpu_types[args.cpu])

        # Create a cache hierarchy (unless we are simulating a
        # functional CPU in atomic memory mode) for the CPU cluster
        # and connect it to the shared memory bus.
        if self.cpu_cluster.memoryMode() == "timing":
            self.cpu_cluster.addL1()
            self.cpu_cluster.addL2(self.cpu_cluster.clk_domain)
        self.cpu_cluster.connectMemSide(self.membus)

        # Tell gem5 about the memory mode used by the CPUs we are
        # simulating.
        self.mem_mode = self.cpu_cluster.memoryMode()

    def numCpuClusters(self):
        return len(self._clusters)

    def addCpuCluster(self, cpu_cluster, num_cpus):
        assert cpu_cluster not in self._clusters
        assert num_cpus > 0
        self._clusters.append(cpu_cluster)
        self._num_cpus += num_cpus

    def numCpus(self):
        return self._num_cpus

def get_processes(cmd):
    """Interprets commands to run and returns a list of processes"""

    cwd = os.getcwd()
    multiprocesses = []
    for idx, c in enumerate(cmd):
        argv = shlex.split(c)
        if argv[0] == 'perlbench':
            print '--> perlbench'
            process = spec06_benchmarks.perlbench
        elif argv[0] == 'bzip2':
            print '--> bzip2'
            process = spec06_benchmarks.bzip2
        elif argv[0] == 'gcc':
            print '--> gcc'
            process = spec06_benchmarks.gcc
        elif argv[0] == 'bwaves':
            print '--> bwaves'
            process = spec06_benchmarks.bwaves
        elif argv[0] == 'gamess':
            print '--> gamess'
            process = spec06_benchmarks.gamess
        elif argv[0] == 'mcf':
            print '--> mcf'
            process = spec06_benchmarks.mcf
        elif argv[0] == 'milc':
            print '--> milc'
            process = spec06_benchmarks.milc
        elif argv[0] == 'zeusmp':
            print '--> zeusmp'
            process = spec06_benchmarks.zeusmp
        elif argv[0] == 'gromacs':
            print '--> gromacs'
            process = spec06_benchmarks.gromacs
        elif argv[0] == 'cactusadm':
            print '--> cactusadm'
            process = spec06_benchmarks.cactusadm
        elif argv[0] == 'leslie3d':
            print '--> leslie3d'
            process = spec06_benchmarks.leslie3d
        elif argv[0] == 'namd':
            print '--> namd'
            process = spec06_benchmarks.namd
        elif argv[0] == 'gobmk':
            print '--> gobmk'
            process = spec06_benchmarks.gobmk
        elif argv[0] == 'dealii':
            print '--> dealii'
            process = spec06_benchmarks.dealii
        elif argv[0] == 'soplex':
            print '--> soplex'
            process = spec06_benchmarks.soplex
        elif argv[0] == 'povray':
            print '--> povray'
            process = spec06_benchmarks.povray
        elif argv[0] == 'calculix':
            print '--> calculix'
            process = spec06_benchmarks.calculix
        elif argv[0] == 'hmmer':
            print '--> hmmer'
            process = spec06_benchmarks.hmmer
        elif argv[0] == 'sjeng':
            print '--> sjeng'
            process = spec06_benchmarks.sjeng
        elif argv[0] == 'gemsfdtd':
            print '--> gemsfdtd'
            process = spec06_benchmarks.gemsfdtd
        elif argv[0] == 'libquantum':
            print '--> libquantum'
            process = spec06_benchmarks.libquantum
        elif argv[0] == 'h264ref':
            print '--> h264ref'
            process = spec06_benchmarks.h264ref
        elif argv[0] == 'tonto':
            print '--> tonto'
            process = spec06_benchmarks.tonto
        elif argv[0] == 'lbm':
            print '--> lbm'
            process = spec06_benchmarks.lbm
        elif argv[0] == 'omnetpp':
            print '--> omnetpp'
            process = spec06_benchmarks.omnetpp
        elif argv[0] == 'astar':
            print '--> astar'
            process = spec06_benchmarks.astar
        elif argv[0] == 'wrf':
            print '--> wrf'
            process = spec06_benchmarks.wrf
        elif argv[0] == 'sphinx3':
            print '--> sphinx3'
            process = spec06_benchmarks.sphinx3
        elif argv[0] == 'xalancbmk':
            print '--> xalancbmk'
            process = spec06_benchmarks.xalancbmk
        elif argv[0] == 'specrand_i':
            print '--> specrand_i'
            process = spec06_benchmarks.specrand_i
        elif argv[0] == 'specrand_f':
            print '--> specrand_f'
            process = spec06_benchmarks.specrand_f
        else:
            print "no recognized spec2006 benchmark selected! exiting."
            sys.exit(1)

        my_process = Process(pid=100 + idx, 
                cwd=process.cwd, 
                cmd=process.cmd,
                executable=process.executable,
                input=process.input,
                output=process.output)
        

        print "info: %d. command and arguments: %s" % (idx + 1, my_process.cmd)
        multiprocesses.append(my_process)

    return multiprocesses


def create(args):
    ''' Create and configure the system object. '''

    system = SimpleSeSystem(args)

    # Tell components about the expected physical memory ranges. This
    # is, for example, used by the MemConfig helper to determine where
    # to map DRAMs in the physical address space.
    print args.mem_size
    system.mem_ranges = [ AddrRange(start=0, size=args.mem_size) ]

    # Configure the off-chip memory system.
    MemConfig.config_mem(args, system)

    # Parse the command line and get a list of Processes instances
    # that we can pass to gem5.
    processes = get_processes(args.commands_to_run)
    if len(processes) != args.num_cores:
        print "Error: Cannot map %d command(s) onto %d " \
            "CPU(s)" % (len(processes), args.num_cores)
        sys.exit(1)

    # Assign one workload to each CPU
    for cpu, workload in zip(system.cpu_cluster.cpus, processes):
        cpu.workload = workload

    return system

def addIPSystem(system, args):
    default_cfg = ConfigParser.SafeConfigParser()
    default_cfg_file = os.path.join(
            os.path.dirname(os.path.realpath(__file__)), "ip_template.cfg")
    default_cfg.read(default_cfg_file)
    defaults = dict(i for i in default_cfg.items("DEFAULT"))

    config = ConfigParser.SafeConfigParser(defaults)
    config.read(args.IP_cfg_file)
    print args.IP_cfg_file
    ip_param_list = config.sections()
    ip_param = ip_param_list[0]
    print ip_param
    if not ip_param:
        fatal("No multimedia IPs were specified!")
    
    # Frame sizes specified in playback flow
    fsize_in = config.get(ip_param, "full_frame_size_in")
    esize_in = config.get(ip_param, "encoded_frame_size_in")
    fullFrameSize_in = "%sB" % (fsize_in)
    encodedFrameSize_in = "%sB" % (esize_in)
   
    # Frame sizes specified in record flow
    fsize_out = config.get(ip_param, "full_frame_size_out")
    esize_out = config.get(ip_param, "encoded_frame_size_out")
    fullFrameSize_out = "%sB" % (fsize_out)
    encodedFrameSize_out = "%sB" % (esize_out)
   
    # Frame Coding ratio
    coding_ratio_ = config.getint(ip_param, "coding_ratio")

    # IP's input/output buffer size
    in_buf_size = config.getint(ip_param, "input_buffer_size")
    out_buf_size = config.getint(ip_param, "output_buffer_size")
    in_buffer_size = "%dkB" %(in_buf_size)
    out_buffer_size = "%dkB" % (out_buf_size)
    
    # Slice mode enable
    if config.getboolean(ip_param, "slice_mode") :
        slice_mode = "true"
    else :
        slice_mode = "false"
    totalSliceCountIn = config.getint(ip_param, "totalSliceCount_in")
    totalSliceCountOut = config.getint(ip_param, "totalSliceCount_out")

    # Shared Response Queue in Governor
    respQueue = IPPacketQueue()
    
    # Multimedia Governor Configuration
    gov_cycle_time = config.getfloat(ip_param, "gov_cycle_time")
    gov_clock = "%1.3fGHz" % (1/gov_cycle_time)
    gov_clk_domain = SrcClockDomain(
            clock = gov_clock, voltage_domain = system.cpu_cluster.voltage_domain)
    fps = config.getint(ip_param, "frame_per_seconds")
    app_type = config.get(ip_param, "app_type")
    system.governor = LGGovernor(
            clk_domain = gov_clk_domain, 
            frame_per_seconds = 60,
            response_queue = respQueue, 
            flow_type = app_type,
            sliceMode = slice_mode,
            full_frame_size_in = fullFrameSize_in,
            encoded_frame_size_in = encodedFrameSize_in,
            full_frame_size_out = fullFrameSize_out,
            encoded_frame_size_out = encodedFrameSize_out,
            totalSliceCount_in = totalSliceCountIn,
            totalSliceCount_out = totalSliceCountOut
            )

    # Video Decoder Configuration
    vd_cycle_time = config.getfloat(ip_param, "vd_cycle_time")
    vd_clock = "%1.3fGHz" % (1/vd_cycle_time)
    vd_compute_time = config.getint(ip_param, "vd_compute_time")
    vd_clk_domain = SrcClockDomain(
            clock = vd_clock, voltage_domain = system.cpu_cluster.voltage_domain)
    system.governor.video_decoder = LGVideoDecoder(
            clk_domain = vd_clk_domain,
            response_queue = respQueue,
            input_buffer_size = in_buffer_size,
            output_buffer_size = out_buffer_size,
            full_frame_size = fullFrameSize_in,
            encoded_frame_size = encodedFrameSize_in,
            coding_ratio = coding_ratio_,
            processing_latency = vd_compute_time )

    # Video Encoder Configuration
    ve_cycle_time = config.getfloat(ip_param, "ve_cycle_time")
    ve_clock = "%1.3fGHz" % (1/ve_cycle_time)
    ve_compute_time = config.getint(ip_param, "ve_compute_time")
    ve_clk_domain = SrcClockDomain(
            clock = ve_clock, voltage_domain = system.cpu_cluster.voltage_domain)
    system.governor.video_encoder = LGVideoEncoder(
            clk_domain = ve_clk_domain,
            response_queue = respQueue,
            input_buffer_size = in_buffer_size,
            output_buffer_size = out_buffer_size,
            full_frame_size = fullFrameSize_out,
            encoded_frame_size = encodedFrameSize_out,
            coding_ratio = coding_ratio_,
            processing_latency = ve_compute_time )

    # Video Display Controller Configuration
    dc_cycle_time = config.getfloat(ip_param, "dc_cycle_time")
    dc_clock = "%1.3fGHz" % (1/dc_cycle_time)
    dc_compute_time = config.getint(ip_param, "dc_compute_time")
    dc_clk_domain = SrcClockDomain(
            clock = dc_clock, voltage_domain = system.cpu_cluster.voltage_domain)
    system.governor.video_display = LGVideoDisplay(
            clk_domain = dc_clk_domain,
            response_queue = respQueue,
            input_buffer_size = in_buffer_size,
            output_buffer_size = out_buffer_size,
            full_frame_size = fullFrameSize_in,
            encoded_frame_size = encodedFrameSize_in,
            coding_ratio = coding_ratio_,
            processing_latency = dc_compute_time )

    # Video Camera Controller Configuration
    cam_cycle_time = config.getfloat(ip_param, "cam_cycle_time")
    cam_clock = "%1.3fGHz" % (1/cam_cycle_time)
    cam_compute_time = config.getint(ip_param, "cam_compute_time")
    cam_clk_domain = SrcClockDomain(
            clock = cam_clock, voltage_domain = system.cpu_cluster.voltage_domain)
    system.governor.video_camera = LGVideoCamera(
            clk_domain = cam_clk_domain,
            response_queue = respQueue,
            input_buffer_size = in_buffer_size,
            output_buffer_size = out_buffer_size,
            full_frame_size = fullFrameSize_out,
            encoded_frame_size = encodedFrameSize_out,
            coding_ratio = coding_ratio_,
            processing_latency = cam_compute_time )

    # Video Camera Controller Configuration
    nw_cycle_time = config.getfloat(ip_param, "nw_cycle_time")
    nw_clock = "%1.3fGHz" % (1/nw_cycle_time)
    nw_compute_time = config.getint(ip_param, "nw_compute_time")
    nw_clk_domain = SrcClockDomain(
            clock = nw_clock, voltage_domain = system.cpu_cluster.voltage_domain)
    system.governor.video_nw = LGVideoNW(
            clk_domain = nw_clk_domain,
            response_queue = respQueue,
            full_frame_size = fullFrameSize_out,
            encoded_frame_size = encodedFrameSize_out,
            processing_latency = nw_compute_time )

    if args.cache_mapping :
        system.governor.video_decoder.read_port = system.membus.slave
        system.governor.video_decoder.write_port = system.cpu_cluster.toL2Bus.slave
        system.governor.video_encoder.read_port = system.membus.slave
        system.governor.video_encoder.write_port = system.membus.slave
        system.governor.video_display.read_port = system.cpu_cluster.toL2Bus.slave
        system.governor.video_camera.write_port = system.membus.slave
        system.governor.video_nw.dma_port = system.membus.slave
    else:
        system.governor.video_decoder.read_port = system.membus.slave
        system.governor.video_decoder.write_port = system.membus.slave
        system.governor.video_encoder.read_port = system.membus.slave
        system.governor.video_encoder.write_port = system.membus.slave
        system.governor.video_display.read_port = system.membus.slave
        system.governor.video_camera.write_port = system.membus.slave
        system.governor.video_nw.dma_port = system.membus.slave


def main():
    parser = argparse.ArgumentParser(epilog=__doc__)

    parser.add_argument("commands_to_run", metavar="command(s)", nargs='*',
                        help="Command(s) to run")
    parser.add_argument("--cpu", type=str, choices=cpu_types.keys(),
                        default="atomic",
                        help="CPU model to use")
    parser.add_argument("--cpu-freq", type=str, default="4GHz")
    parser.add_argument("--num-cores", type=int, default=1,
                        help="Number of CPU cores")
    parser.add_argument("--mem-type", default="DDR3_1600_8x8",
                        choices=MemConfig.mem_names(),
                        help = "type of memory to use")
    parser.add_argument("--mem-channels", type=int, default=2,
                        help = "number of memory channels")
    parser.add_argument("--mem-ranks", type=int, default=None,
                        help = "number of memory ranks per channel")
    parser.add_argument("--mem-size", action="store", type=str,
                        default="2GB",
                        help="Specify the physical memory size")

    ##
    parser.add_argument("--add_IP", action="store_true")
    parser.add_argument("--cache_mapping", action="store_true")
    parser.add_argument("--IP_cfg_file", default=None,
                        help="Multimedia IP configuration file")
    parser.add_argument("--l2_policy", type=str, default=None,
                        help="L2 Cache Replacement Policy ")
    args = parser.parse_args()

    # Create a single root node for gem5's object hierarchy. There can
    # only exist one root node in the simulator at any given
    # time. Tell gem5 that we want to use syscall emulation mode
    # instead of full system mode.
    root = Root(full_system=False)

    # Populate the root node with a system. A system corresponds to a
    # single node with shared memory.
    root.system = create(args)

    #
    #
    if args.add_IP :
        addIPSystem(root.system, args)

    if args.l2_policy == "RRIP":
        root.system.cpu_cluster.l2.tags = RRIP()
    elif args.l2_policy == "PDPP":
        root.system.cpu_cluster.l2.tags = PDPP()
    elif args.l2_policy == "PDPS":
        root.system.cpu_cluster.l2.tags = PDPS()
    elif args.l2_policy == "DCS":
        root.system.cpu_cluster.l2.tags = DCS()


    # Instantiate the C++ object hierarchy. After this point,
    # SimObjects can't be instantiated anymore.
    m5.instantiate()

    # Start the simulator. This gives control to the C++ world and
    # starts the simulator. The returned event tells the simulation
    # script why the simulator exited.
    event = m5.simulate(500000000000)

    # Print the reason for the simulation exit. Some exit codes are
    # requests for service (e.g., checkpoints) from the simulation
    # script. We'll just ignore them here and exit.
    print event.getCause(), " @ ", m5.curTick()

    m5.stats.reset()
    event = m5.simulate()
    print event.getCause(), " @ ", m5.curTick()

    sys.exit(event.getCode())


if __name__ == "__m5_main__":
    main()
