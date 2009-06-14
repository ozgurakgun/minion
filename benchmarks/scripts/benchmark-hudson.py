# First attempt at a benchmarking script
import sys, os, timeit
from threading import Thread
program = sys.argv[1]
benchname = sys.argv[2]
totaltime = 0
nodes = 0

class BThread (Thread):
  def __init__ (self,benchmarks):
    Thread.__init__(self)
    self.benchmarks = benchmarks
    self.totaltime = 0
    self.nodes = 0

  def run (self):
    for name in self.benchmarks:
      if name.endswith('.minion') or name.endswith('.minion.bz2') or name.endswith('.minion.gz'):
        print 'Running ' + name
        timer = timeit.Timer('os.system("'+program+' '+name+' > '+name+'.benchmark.out")','import os')
        time = timer.timeit(1)
        if(time < 5.0):
          # This is a fast experiment
          time = timer.repeat(3,5)
          self.totaltime += min(time)/10.0
        else:
          time = timer.repeat(3,1)
          self.totaltime += min(time)
        infile = open(name+'.benchmark.out')
        for line in infile:
          if(line.startswith("Total Nodes:")):
            self.nodes += int(line.partition(':')[2])
    

benchmarks = sys.argv[3:]

# split across 2 threads -- TODO: make generic
workers = []
current = BThread([elem for elem in benchmarks if benchmarks.index(elem) % 2 == 0])
workers.append(current)
current.start()
current = BThread([elem for elem in benchmarks if benchmarks.index(elem) % 2 == 1])
workers.append(current)
current.start()

for worker in workers:
  worker.join()
  totaltime += worker.totaltime
  nodes += worker.nodes

FILE = open(benchname + ".benchmark", "w")
FILE.write('YVALUE=%.3f' % totaltime)
FILE.close
FILE = open(benchname + ".nodes.benchmark", "w")
FILE.write('YVALUE=%d' % nodes)
FILE.close
