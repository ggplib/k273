import os
import sys
from ggpzero.util import runprocs

from twisted.internet import reactor

cmd = "rsync -vaz --delete /home/rxe/working/ggpzero/data/breakthrough/ rxe@sim:working/ggpzero/data/breakthrough"
watch_path = "/home/rxe/working/ggpzero/data/breakthrough/generations/"


class WatchDir(object):
    def __init__(self):
        self.cmds = None
        self.last_time = os.stat(watch_path).st_mtime
        reactor.callLater(0.1, self.check)

    def check(self):
        print ".",
        sys.stdout.flush()
        mtime = os.stat("/home/rxe/working/ggpzero/data/reversi/generations/").st_mtime
        if mtime > self.last_time + 0.01:
            print "directory changed", watch_path
            self.last_time = mtime
            self.run()
        else:
            reactor.callLater(5, self.check)

    def done(self):
        print "DONE sync"
        reactor.callLater(5, self.check)

    def run(self):
        self.cmds_running = runprocs.RunCmds([cmd], cb_on_completion=self.done, max_time=180.0)
        self.cmds_running.spawn()


if __name__ == "__main__":
    from ggplib.util.init import setup_once
    setup_once("runner")

    watch_dir = WatchDir()
    reactor.run()
