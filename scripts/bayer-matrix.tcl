#! /usr/bin/env tclsh

set bm8 {
     0 48 12 60  3 51 15 63
    32 16 44 28 35 19 47 31
     8 56  4 52 11 59  7 55
    40 24 36 20 43 27 39 23
     2 50 14 62  1 49 13 61
    34 18 46 30 33 17 45 29
    10 58  6 54  9 57  5 53
    42 26 38 22 41 25 37 21
}

set n 8
set size [expr { $n * $n }]

set fmt [lmap x $bm8 {
    format %2i.0/%u $x $size
}]

for {set i 0} {$i < $size} {incr i $n} {
    lappend lines [join [lrange $fmt $i [expr { $i + $n - 1 }]] {, }]
}

puts "\n    [join $lines ",\n    "]"
