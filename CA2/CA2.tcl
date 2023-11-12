if { $argc != 2 } {
   puts "The CA2.tcl script requires bandwidth and packet size. \n For example, 'ns CA2.tcl 1.5 1000' \n Please try again!"
    return 0;
} else {
    
   if { [lindex $argv 0] == "1.5" } {
        set Bandwidth 1.5Mb
    } else {
      if { [lindex $argv 0] == "55" } {
        set Bandwidth 55Mb
        } else {
            if { [lindex $argv 0] == "155" } {
            set Bandwidth 155Mb
        } else {
            puts "The CA2.tcl script requires bandwidth. \n For example, 'ns CA2.tcl 1.5' \n Please try again."
            return 0;
        }
    }
  } 
}

set arg2 [lindex $argv 1]
set packet_size [expr $arg2]
Mac/802_11 set dataRate_ $Bandwidth

set opt(chan)           Channel/WirelessChannel  ;# channel type
set opt(prop)           Propagation/TwoRayGround ;# radio-propagation model
set opt(netif)          Phy/WirelessPhy          ;# network interface type
set opt(mac)            Mac/802_11               ;# MAC type
set opt(ifq)            Queue/DropTail/PriQueue  ;# interface queue type
set opt(ll)             LL                       ;# link layer type
set opt(ant)            Antenna/OmniAntenna      ;# antenna model
set opt(ifqlen)         50                       ;# max packet in ifq
set opt(bottomrow)      9                        ;# number of bottom-row nodes
set opt(spacing)        200                      ;# spacing between bottom-row nodes
set opt(mheight)        150                      ;# height of moving node above bottom-row nodes
set opt(brheight)       50                       ;# height of bottom-row nodes from bottom edge
set opt(x)              300                      ;# x coordinate of topology
set opt(y)              300                      ;# y coordinate of topology
set opt(adhocRouting)   AODV                     ;# routing protocol
set opt(finish)         100                      ;# time to stop simulation
set opt(speed)          [expr 1.0*$opt(x)/$opt(finish)]ظ

set ns [new Simulator]

$ns use-newtrace
set nf [open out.nam w]
$ns namtrace-all-wireless $nf $opt(x) $opt(y)
set tf [open out.tr w]
$ns trace-all $tf

proc finish {} {
    global ns tf nf
    $ns flush-trace
    close $tf
    close $nf
    exec nam out.nam &
    exit 0
}

set topo [new Topography]
$topo load_flatgrid $opt(x) $opt(y)

create-god [expr $opt(bottomrow) + 1]

set chan1 [new $opt(chan)]

proc Err {} {
	set err [new ErrorModel]
    $err set rate_ 0.00001
	$err unit packet
	return $err
}

$ns node-config -adhocRouting $opt(adhocRouting) \
                -llType       $opt(ll) \
                -macType      $opt(mac) \
                -ifqType      $opt(ifq) \
                -ifqLen       $opt(ifqlen) \
                -antType      $opt(ant) \
                -propType     $opt(prop) \
                -phyType      $opt(netif) \
                -channel      $chan1 \
                -topoInstance $topo \
                -wiredRouting OFF \
                -agentTrace   ON \
                -IncomingErrProc Err \
                -OutgoingErrProc Err \
                -routerTrace  ON \
                -macTrace     OFF


# A
set rownode(0) [$ns node]
$rownode(0) set X_ 200
$rownode(0) set Y_ 350
$rownode(0) set Z_ 0
$rownode(0) label "A"

$ns at 0.0 "$rownode(0) color red"
$rownode(0) color red
# B
set rownode(1) [$ns node]
$rownode(1) set X_ 50
$rownode(1) set Y_ 200
$rownode(1) set Z_ 0
$rownode(1) label "B"
# C
set rownode(2) [$ns node]
$rownode(2) set X_ 300
$rownode(2) set Y_ 275
$rownode(2) set Z_ 0
$rownode(2) label "C"
# D
set rownode(3) [$ns node]
$rownode(3) set X_ 200
$rownode(3) set Y_ 0
$rownode(3) set Z_ 0
$rownode(3) label "D"

$ns at 0.0 "$rownode(3) color red"
$rownode(3) color red
# E
set rownode(4) [$ns node]
$rownode(4) set X_ 300
$rownode(4) set Y_ 125
$rownode(4) set Z_ 0
$rownode(4) label "E"
# F
set rownode(5) [$ns node]
$rownode(5) set X_ 450
$rownode(5) set Y_ 125
$rownode(5) set Z_ 0
$rownode(5) label "F"
# G
set rownode(6) [$ns node]
$rownode(6) set X_ 450
$rownode(6) set Y_ 275
$rownode(6) set Z_ 0
$rownode(6) label "G"
# H
set rownode(7) [$ns node]
$rownode(7) set X_ 600
$rownode(7) set Y_ 275
$rownode(7) set Z_ 0
$rownode(7) label "H"


$ns at 0.0 "$rownode(7) color purple"
$rownode(7) color purple
# L
set rownode(8) [$ns node]
$rownode(8) set X_ 600
$rownode(8) set Y_ 125
$rownode(8) set Z_ 0
$rownode(8) label "L"

$ns at 0.0 "$rownode(8) color purple"
$rownode(8) color purple


for {set i 0} {$i < $opt(bottomrow)} {incr i} {
    $ns initial_node_pos $rownode($i) 10
}


set tcp1 [new Agent/TCP]
$tcp1 set class_ 2
$tcp1 set packetSize_ $packet_size
set sink1 [new Agent/TCPSink]
$ns attach-agent $rownode(0) $tcp1
$ns attach-agent $rownode(8) $sink1
$ns connect $tcp1 $sink1
set ftp1 [new Application/FTP]
$ftp1 attach-agent $tcp1
$ns at 0 "$ftp1 start"

set tcp2 [new Agent/TCP]
$tcp2 set class_ 2
$tcp2 set packetSize_ $packet_size
set sink2 [new Agent/TCPSink]
$ns attach-agent $rownode(3) $tcp2        
$ns attach-agent $rownode(8) $sink2
$ns connect $tcp2 $sink2
set ftp2 [new Application/FTP]
$ftp2 attach-agent $tcp2
$ns at 0 "$ftp2 start"

set tcp3 [new Agent/TCP]
$tcp3 set class_ 2
$tcp3 set packetSize_ $packet_size
set sink3 [new Agent/TCPSink]
$ns attach-agent $rownode(0) $tcp3       
$ns attach-agent $rownode(7) $sink3
$ns connect $tcp3 $sink3
set ftp3 [new Application/FTP]
$ftp3 attach-agent $tcp3
$ns at 0 "$ftp3 start"

set tcp4 [new Agent/TCP]
$tcp4 set class_ 2
$tcp4 set packetSize_ $packet_size
set sink4 [new Agent/TCPSink]
$ns attach-agent $rownode(3) $tcp4       
$ns attach-agent $rownode(7) $sink4
$ns connect $tcp4 $sink4
set ftp4 [new Application/FTP]
$ftp4 attach-agent $tcp4
$ns at 0 "$ftp4 start"


$ns  at 100.0 "finish"
# begin simulation
$ns run
