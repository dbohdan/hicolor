#! /usr/bin/env tclsh

package require textutil

set varDeclTemplate {static const uint8_t %s[] = {%s};}

proc table {from to} {
    set ratio [expr { $to * 1.0 / $from }]

    for {set i 0} {$i < $from} {incr i} {
        lappend table [expr { int($i * 1.0 / $from * ($to + $ratio) ) }]
    }

    return $table
}

proc format-table {name table} {
    set lines [textutil::adjust [join $table {, }]]
    set indented [join [split $lines \n] "\n    "]

    return [format $::varDeclTemplate $name "\n    $indented\n"]
}

foreach {from to} {256 32 256 64 32 256 64 256} {
    dict set tables hicolor_${from}_to_$to [table $from $to]
}

puts [join [lmap {key value} $tables {
    format-table $key $value
}] \n\n]
