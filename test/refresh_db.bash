sudo -u postgres psql postgres << _EOF_
drop database fsanitycheck2;
create database fsanitycheck2;
_EOF_

sudo -u postgres psql fsanitycheck2 < ../db/fsanitycheck.sql

