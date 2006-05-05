#!/usr/bin/perl -w
#

#  This script updates the variable INSTALL_FILE_LIST in the
#  `Makefile.in' file in this directory.  The script sets
#  INSTALL_FILE_LIST to the list of non-Makefile-files listed in the
#  CVS/Entries file.  The former value of INSTALL_FILE_LIST is lost,
#  but a backup copy of Makefile.in is available in `Makefile.in.$$~'
#  where $$ is the process-id.

#  Load modules.  man <module-name> for more info.
use strict;
use IO::File;

#  The Makefile variable to update
my $update_macro = 'INSTALL_FILE_LIST';

#  @files will contain the list of files to add to the Makefile.in
my @files = ();

#  @exclude contains the list of files NOT to add to Makefile.in
my @exclude = qw(configure configure.in
                 Makefile.in Makefile.Rules.in Makefile.Template
                 update-Makefile.pl);

#  Convert the exclude array into a hash for quick lookup
my %exclude = ();
foreach (@exclude) {
    $exclude{$_} = 1;
}

#  Populate @files by reading the CVS/Entries file in this directory
&readEntries('.');

#  Sort files
@files = sort @files;

#  Rename `Makefile.in' to `Makefile.in.$$~'
my $new = 'Makefile.in';
my $old = "$new.$$~";
rename( $new, $old ) ||
    die "Cannot rename $new: $!";

#  Open the files
my $NEW = new IO::File $new, 'w';
my $OLD = new IO::File $old;
die "Cannot open $new: $!" unless defined $NEW;
die "Cannot open $old: $!" unless defined $OLD;

#  Read in the old version.  We do a straight copy until we find
#  $update_macro, at which point we ignore everthing in the old
#  version until the end of $update_macro definition, and we insert
#  the list of files we found (in @files).
while( <$OLD> ) {
    $NEW->print( $_);
    next unless /^$update_macro\s*=/o;
    #  Ignore everything in $OLD until the end of $update_macro
    while( <$OLD> ) {
        next if m,\\,;
        last;
    }
    #  The last line in the $update_macro definition does not end in a
    #  backslash, so we have to treat it specially
    my $last = pop @files;
    #  Print each file in @files
    foreach my $f ( @files ) {
        $NEW->print( "\t", $f, " \\\n" );
    }
    #  Now print the last one
    $NEW->print( "\t", $last, "\n" );
}

#  Close the files
$OLD->close() || die "Cannot close $old: $!";
$NEW->close() || die "Cannot close $new: $!";

#  All done!
exit;

#  Add most of the files in the CVS/Entries file to the global @files
#  variable (ignore those in %exclude).  The argument to readEntries
#  is the directory which the CVS/Entries file applies to
sub readEntries {
    my($dir) = @_;
    # localize globals
    local($_,$!,$1);
    # open the CVS/Entries file
    my $entry = new IO::File "CVS/Entries";
    die "Cannot open $_/Entries: $!" unless defined $entry;
    # For each line in $entry, split the line on slash and check to
    # see if the file name is in %exclude; if not, add it to the
    # global variable @files
    while( <$entry> ) {
        my($empty,$file,$version,$data,$misc) = split( '/', $_ );
        next if defined $exclude{$file};
        push @files, $file;
    }
    $entry->close() || die "Cannot close $dir/CVS/Entries: $!";
}
