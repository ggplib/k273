if [ -z "$K273_PATH" ]; then
    export K273_PATH=`python2 -c "import os.path as p; print p.dirname(p.dirname(p.abspath('$BASH_SOURCE')))"`
    echo "Automatically setting \$K273_PATH to" $K273_PATH
else

    echo "K273 set to" $K273_PATH
fi

export LD_LIBRARY_PATH=$K273_PATH/build/lib:$LD_LIBRARY_PATH

