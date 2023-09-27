# many2one

Usage 

-h     help

-i    io_gen_threads 1-16 default 4

-c     io_gen cpus x,y,z       coma seperated list, default is no cpu affinity

-s     cli_cpu     default is no cpu affinity

-e   emulator cpu    default is no cpu affinity

-t   total em messages, default 1M

-a queue type

0 default, spinlock not aligned

1 spinlock aligned

2 spinlock only akigned

3 mutex not aligned

4 mutex aligned