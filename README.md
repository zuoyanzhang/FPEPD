# FPEPD
**Description**

FPEPD is an error detection tool using Sub-domain parallel search.

**Installation**
· Dependence on environment
MPFR, GMP library. You can install by executing the following command.

```
$ sudo apt-get update
$ sudo apt-get upgrade
$ sudo apt-get install libgmp-dev libmpfr-dev
```

**Instructions**

FPEPD is a command-line tool that you can use on Ubuntu.
```
$ git clone https://github.com/zuoyanzhang/FPEPD.git
$ cd FPEPD
```
You can then execute the follow command to perform error detection.
```
$ make
$ bin/geneMPFR.exe "expressions"
$ cd pdmethod
$ make
$ bin/FPEPD.exe x_l x_r // single-variable expressions range
or
$ bin/FPEPD.exe x1_l x1_r x2_l x2_r // two-variable expressions range
```
**Here is an example**

If you want to detect the error of the expression (x * x * x) / (x - 1.231) in the domain [0,20], execute:
```
$ cd FPEPD
$ make
$ bin/geneMPFR.exe "(x * x * x) / (x - 1.231)"
$ cd pdmethod
$ make
$ bin/FPEPD.exe 0 20
```
The output will be:
```
Final Iteration - Maximum ULP Error: 1.89, at X: 19.4134426020451940, and Bit Error: 1.53
```
If you want to detect the error of the expression (x * y) / (x - y) in the domain [0,10]x[10,20], execute:
```
$ cd FPEPD
$ make
$ bin/geneMPFR.exe "(x * y) / (x - y)"
$ cd pdmethod
$ make
$ bin/FPEPD.exe 0 10 10 20
```
The output will be:
```
Final Iteration - Maximum ULP Error: 0.99, at X1: 2.4162173433740719, at X2: 10.5567159645151776, and Bit Error: 0.99
```


