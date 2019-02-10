# 12306ticket
  <p>&nbsp;&nbsp;This application is develop for linux platform and it is intend to run with gtk3, it also depend on libcurl which is a client url library whiten by C.Please check your platform dependencies before compile this code or run this application.Remeber modify <em>tickethelper.conf</em> to your own setting.</p>
  For installing gtk3, on Fedora:<pre>sudo dnf install gtk3 gtk3-devel</pre>
  On Debian or Ubuntu:<pre>sudo apt-get install libgtk-3-dev</pre>
  On Arch Linux:<pre>sudo pacman -Sy gtk3</pre>
  On OpenSUSE:<pre>sudo zypper install gtk3 gtk3-dev</pre>
  On Linux Mint:<pre>gem install gtk3 gtk3-dev</pre>
  <br />
  <h3>How to compile and run?</h3>
  <p>Copy this repository to local</p>
  <pre>git clone https://github.com/liujianjia/12306ticket.git</pre>
  <p>or download zip package and extract it</p>
  <p>Enter project directory</p>
  <pre>cd 12306helper</pre>
  <p>Build project</p>
  <pre>make</pre>
  <p>If everything is ok, there is an executable file which name is <em>tickethelper</em> generated<p>
  <p>Use your favorite text editor modify <em>tickethelper.conf</em> to your own setting, such as</p>
  <pre>vim tickethelper.conf</pre>
  <p>Then start application, good luck!</p>
  <pre>./tickethelper</pre>
  <br />
  Available command line options:
  <pre>
  -c, --config
    specify configuration file.<br />
  -h, --help
    print this message and exit.<br />
  -q, --queit
    queit mode, don't output each train information.<br />
  -Q, --query-only
    query only mode, only query ticket information and disable auto order
  ticket function.<br />
  -v, --verbose
    enable verbose mode, output more detail information about connection and debug 
  information.<br />
  -V, --version
    print application version and exit.
  </pre>
