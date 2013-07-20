debug_option = 0
def debug( debugvar, debugstring ):
   if debug_option == 1:
       print str(debugvar) + " = " + str(debugstring)
