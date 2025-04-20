#!/bin/bash
otool_list_load_commands() {

    otool -l ./empty > tmp.txt || exit

}

otool_list_load_commands