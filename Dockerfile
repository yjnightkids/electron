FROM libchromiumcontent-linux:latest

# Install node.js
RUN curl -sL https://deb.nodesource.com/setup_6.x | bash -
RUN apt-get update && apt-get install -y --force-yes nodejs

# Install wget used by crash reporter
RUN apt-get install -y --force-yes  wget

# Add xvfb init script
ADD tools/xvfb-init.sh /etc/init.d/xvfb
RUN chmod a+x /etc/init.d/xvfb
