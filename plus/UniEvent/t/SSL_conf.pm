use strict;

package SSL_conf;

use constant serv_cert => 'cppTests/cert.pem';

sub client {
  return (SSL_ca_file => serv_cert);
}

sub server {
  return (SSL_cert_file => serv_cert, SSL_key_file => 'cppTests/key.pem');
}
