import m5
from m5.objects import *

m5.util.addToPath('../common')

binary_dir = '/home/yongwoo/CPU2006/'
data_dir = binary_dir

# 400.perlbench
perlbench = Process()
perlbench.cwd = binary_dir + '400.perlbench/run/run_base_ref_arm.0000/'
perlbench.executable = perlbench.cwd + 'perlbench_base.arm'
perlbench.cmd = [perlbench.executable] + ['-I./lib', 'checkspam.pl', '2500', '5', '25', '11', '150', '1', '1', '1', '1']
perlbench.output = perlbench.cwd+'400.out'

#401.bzip2
bzip2 = Process()
bzip2.cwd = binary_dir + '401.bzip2/run/run_base_ref_arm.0000/'
bzip2.executable =  bzip2.cwd +'bzip2_base.arm'
data = bzip2.cwd+'input.program'
bzip2.cmd = [bzip2.executable] + [data, '280']
bzip2.output = bzip2.cwd+'401.out'

#403.gcc
gcc = Process()
gcc.cwd = binary_dir + '403.gcc/run/run_base_ref_arm.0000/'
gcc.executable = gcc.cwd +'gcc_base.arm'
data = gcc.cwd +'166.i'
output = gcc.cwd +'166.s'
gcc.cmd = [gcc.executable] + [data]+['-o',output]
gcc.output = gcc.cwd+'403.out'

#410.bwaves
bwaves = Process()
bwaves.cwd = binary_dir + '410.bwaves/run/run_base_ref_arm.0000/'
bwaves.executable =  bwaves.cwd +'bwaves_base.arm'
bwaves.cmd = [bwaves.executable]
bwaves.output = bwaves.cwd+'410.out'

#416.gamess
gamess=Process()
gamess.cwd = binary_dir + '416.gamess/run/run_base_ref_arm.0000/'
gamess.executable =  gamess.cwd + 'gamess_base.arm'
gamess.cmd = [gamess.executable]
gamess.input= gamess.cwd + 'cytosine.2.config'
gamess.output=gamess.cwd + '416.out'

#429.mcf
mcf = Process()
mcf.cwd = binary_dir + '429.mcf/run/run_base_ref_arm.0000/'
mcf.executable = mcf.cwd +'mcf_base.arm'
data = mcf.cwd+'inp.in'
mcf.cmd = [mcf.executable] + [data]
mcf.output = mcf.cwd+'429.out'

#433.milc
milc=Process()
milc.cwd = binary_dir + '433.milc/run/run_base_ref_arm.0000/'
milc.executable = milc.cwd +'milc_base.arm'
stdin=milc.cwd +'su3imp.in'
milc.cmd = [milc.executable]
milc.input=stdin
milc.output=milc.cwd +'433.out'

#434.zeusmp
zeusmp=Process()
zeusmp.cwd = binary_dir+'434.zeusmp/run/run_base_ref_arm.0000/'
zeusmp.executable = zeusmp.cwd + 'zeusmp_base.arm'
zeusmp.cmd = [zeusmp.executable]
zeusmp.output = zeusmp.cwd + '434.out'

#435.gromacs
gromacs = Process()
gromacs.cwd = binary_dir+'435.gromacs/run/run_base_ref_arm.0000/'
gromacs.executable =  gromacs.cwd +'gromacs_base.arm'
data = gromacs.cwd +'gromacs.tpr'
gromacs.cmd = [gromacs.executable] + ['-silent','-deffnm',data,'-nice','0']
gromacs.output = gromacs.cwd +'435.out'

#436.cactusADM
cactusADM = Process()
cactusADM.cwd = binary_dir+'436.cactusADM/run/run_base_ref_arm.0000/'
cactusADM.executable =  cactusADM.cwd +'cactusADM_base.arm'
data = cactusADM.cwd+'benchADM.par'
cactusADM.cmd = [cactusADM.executable] + [data]
cactusADM.output = cactusADM.cwd +'436.out'

# 437.leslie3d
leslie3d = Process()
leslie3d.cwd = binary_dir + '437.leslie3d/run/run_base_ref_arm.0000/'
leslie3d.executable = leslie3d.cwd + 'leslie3d_base.arm'
leslie3d.cmd = [leslie3d.executable]
leslie3d.input = leslie3d.cwd + 'leslie3d.in'
leslie3d.output = leslie3d.cwd + '437.out'

#444.namd
namd = Process()
namd.cwd = binary_dir + '444.namd/run/run_base_ref_arm.0000/'
namd.executable =  namd.cwd +'namd_base.arm'
input= namd.cwd +'namd.input'
namd.cmd = [namd.executable] + ['--input',input,'--iterations','38','--output','namd.out']
namd.output= namd.cwd +'444.stdout'

#445.gobmk
gobmk=Process()
gobmk.cwd = binary_dir + '445.gobmk/run/run_base_ref_arm.0000/'
gobmk.executable =  gobmk.cwd +'gobmk_base.arm'
stdin= gobmk.cwd +'nngs.tst'
gobmk.cmd = [gobmk.executable]+['--quiet','--mode','gtp']
gobmk.input=stdin
gobmk.output=gobmk.cwd +'445.out'

#447.dealII TODO

#450.soplex
soplex=Process()
soplex.cwd = binary_dir + '450.soplex/run/run_base_ref_arm.0000/'
soplex.executable =  soplex.cwd +'soplex_base.arm'
data= soplex.cwd +'ref.mps'
soplex.cmd = [soplex.executable]+['-m3500',data]
soplex.output = soplex.cwd +'450.out'

#453.povray
povray=Process()
povray.cwd = binary_dir + '453.povray/run/run_base_ref_arm.0000/'
povray.executable =  povray.cwd +'povray_base.arm'
data = povray.cwd +'SPEC-benchmark-ref.ini'
povray.cmd = [povray.executable]+[data]
povray.output = povray.cwd +'453.out'

#454.calculix
calculix=Process()
calculix.cwd = binary_dir + '454.calculix/run/run_base_ref_arm.0000/'
calculix.executable =  calculix.cwd + 'calculix_base.arm'
data = calculix.cwd +'hyperviscoplastic'
calculix.cmd = [calculix.executable]+['-i',data]
calculix.output = calculix.cwd +'454.out'

#456.hmmer
hmmer=Process()
hmmer.cwd = binary_dir + '456.hmmer/run/run_base_ref_arm.0000/'
hmmer.executable =  hmmer.cwd +'hmmer_base.arm'
data = hmmer.cwd +'retro.hmm'
hmmer.cmd = [hmmer.executable]+['--fixed', '0', '--mean', '500', '--num', '500000', '--sd', '350', '--seed', '0', data]
hmmer.output = hmmer.cwd +'456.out'

#458.sjeng
sjeng=Process()
sjeng.cwd = binary_dir + '458.sjeng/run/run_base_ref_arm.0000/'
sjeng.executable = sjeng.cwd +'sjeng_base.arm'
data= sjeng.cwd +'ref.txt'
sjeng.cmd = [sjeng.executable]+[data]
sjeng.output = sjeng.cwd +'458.out'

#459.GemsFDTD
GemsFDTD=Process()
GemsFDTD.cwd = binary_dir + '459.GemsFDTD/run/run_base_ref_arm.0000/'
GemsFDTD.executable =  GemsFDTD.cwd +'GemsFDTD_base.arm'
GemsFDTD.cmd = [GemsFDTD.executable]
GemsFDTD.output = GemsFDTD.cwd +'459.out'

#462.libquantum
libquantum=Process()
libquantum.cwd = binary_dir + '462.libquantum/run/run_base_ref_arm.0000/'
libquantum.executable =  libquantum.cwd +'libquantum_base.arm'
libquantum.cmd = [libquantum.executable],'1397','8'
libquantum.output = libquantum.cwd +'462.out'

#464.h264ref
h264ref=Process()
h264ref.cwd = binary_dir + '464.h264ref/run/run_base_ref_arm.0000/'
h264ref.executable =  h264ref.cwd +'h264ref_base.arm'
data = h264ref.cwd + 'foreman_ref_encoder_baseline.cfg'
h264ref.cmd = [h264ref.executable]+['-d',data]
h264ref.output = h264ref.cwd + '464.out'

#470.lbm
lbm=Process()
lbm.cwd = binary_dir + '470.lbm/run/run_base_ref_arm.0000/'
lbm.executable =  lbm.cwd +'lbm_base.arm'
data= lbm.cwd +'100_100_130_ldc.of'
lbm.cmd = [lbm.executable]+['3000', 'reference.dat', '0', '0' ,data]
lbm.output = lbm.cwd + '470.out'

#471.omnetpp
omnetpp=Process()
omnetpp.cwd = binary_dir + '471.omnetpp/run/run_base_ref_arm.0000/'
omnetpp.executable =  omnetpp.cwd +'omnetpp_base.arm'
data=omnetpp.cwd +'omnetpp.ini'
omnetpp.cmd = [omnetpp.executable]+[data]
omnetpp.output = omnetpp.cwd +'471.out'

#473.astar
astar=Process()
astar.cwd = binary_dir + '473.astar/run/run_base_ref_arm.0000/'
astar.executable =  astar.cwd +'astar_base.arm'
astar.cmd = [astar.executable]+['BigLakes2048.cfg']
astar.output = astar.cwd +'473.out'

#481.wrf
wrf=Process()
wrf.cwd = binary_dir + '481.wrf/run/run_base_ref_arm.0000/'
wrf.executable =  wrf.cwd +'wrf_base.arm'
wrf.cmd = [wrf.executable]+['namelist.input']
wrf.output = wrf.cwd +'481.out'

#482.sphinx3
sphinx3=Process()
sphinx3.cwd = binary_dir + '482.sphinx3/run/run_base_ref_arm.0000/'
sphinx3.executable =  sphinx3.cwd +'sphinx_livepretend_base.arm'
sphinx3.cmd = [sphinx3.executable]+['ctlfile', '.', 'args.an4']
sphinx3.output = sphinx3.cwd +'482.out'

#483.xalancbmk TODO

#998.specrand
specrand_i=Process()
specrand_i.cwd = binary_dir + '998.specrand/run/run_base_ref_arm.0000/'
specrand_i.executable = specrand_i.cwd + 'specrand_base.arm'
specrand_i.cmd = [specrand_i.executable] + ['1255432124','234923']
specrand_i.output = specrand_i.cwd +'998.out'

#999.specrand
specrand_f=Process()
specrand_f.cwd = binary_dir + '999.specrand/run/run_base_ref_arm.0000/'
specrand_f.executable = specrand_f.cwd +'specrand_base.arm'
specrand_f.cmd = [specrand_f.executable] + ['1255432124','234923']
specrand_f.output = specrand_f.cwd +'999.out'
