#!/usr/bin/env python

import gtk

class MainGui:

  def on_window1_destroy(self, object, data=None):
    print "quit with cancel"
    gtk.main_quit()

  def on_gtk_quit_activate(self, menuitem, data=None):
    print "quit from menu"
    gtk.main_quit()

  def __init__(self):
    self.gladefile = "gui.glade"
    self.builder = gtk.Builder()
    self.builder.add_from_file(self.gladefile)
    self.builder.connect_signals(self)
    self.window = self.builder.get_object("window1")
    self.window.show()

if __name__ == "__main__":
  main = MainGui()
  gtk.main()
