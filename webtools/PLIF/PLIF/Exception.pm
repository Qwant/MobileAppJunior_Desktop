# -*- Mode: perl; tab-width: 4; indent-tabs-mode: nil; -*-
#
# This file is MPL/GPL dual-licensed under the following terms:
# 
# The contents of this file are subject to the Mozilla Public License
# Version 1.1 (the "License"); you may not use this file except in
# compliance with the License. You may obtain a copy of the License at
# http://www.mozilla.org/MPL/
#
# Software distributed under the License is distributed on an "AS IS"
# basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
# the License for the specific language governing rights and
# limitations under the License.
#
# The Original Code is PLIF 1.0.
# The Initial Developer of the Original Code is Ian Hickson.
#
# Alternatively, the contents of this file may be used under the terms
# of the GNU General Public License Version 2 or later (the "GPL"), in
# which case the provisions of the GPL are applicable instead of those
# above. If you wish to allow use of your version of this file only
# under the terms of the GPL and not to allow others to use your
# version of this file under the MPL, indicate your decision by
# deleting the provisions above and replace them with the notice and
# other provisions required by the GPL. If you do not delete the
# provisions above, a recipient may use your version of this file
# under either the MPL or the GPL.

package PLIF::Exception;
use strict;
use vars qw(@ISA @EXPORT);
require Exporter;
@ISA = qw(Exporter);
@EXPORT = qw(try raise handle with unhandled except finally);

# To use this package, you first have to define your own exceptions:
#
#     package MyException; @ISA = qw(PLIF::Exception);
#     package MemoryException; @ISA = qw(PLIF::Exception);
#     package IOException; @ISA = qw(PLIF::Exception);
#
# You can then use them as follows:
#
#     try {
#         # some code that might raise an exception:
#         raise MyException if $condition;
#         # ... more code ...
#     } handle MemoryException with {
#         my($exception) = @_;
#         raise $exception; # reraise 
#     } handle IOException with {
#         my($exception) = @_;
#         unhandled; # fall through to the following handlers
#     } handle DBError with {
#         my($exception) = @_;
#     } except {
#         my($exception) = @_;
#         # catch all, called if the exception wasn't handled
#     } finally {
#         # always called after try block
#     };


sub syntax($@) {
    my($message, $package, $filename, $line) = @_;
    die "$message at $filename line $line\n";
}

sub create {
    my $class = shift;
    return bless({@_}, $class);
}

sub raise {
    my($exception, @data) = @_;
    if (ref($exception) and $exception->isa('PLIF::Exception')) {
        # if the exception is an object, raise it
        # this is for people doing things like:
        #   raise IOException->create('message' => $!);
        # or:
        #   my $memoryException = MemoryException->create();
        #   # ...
        #   $memoryException->raise();
        die $exception;
    } else {
        # otherwise, assume we were called as a constructor
        # this is for people doing things like:
        #   raise IOException;
        # or:
        #   raise IOException ('message' => $!);
        # or:
        #   IOException->raise('message' => $!);
        syntax "Syntax error in \"raise\": \"$exception\" is not a PLIF::Exception class", caller unless $exception->isa('PLIF::Exception');
        die $exception->create(@data);
    }
}

sub try(&;$) {
    my($code, $continuation) = @_;
    if (defined($continuation) and
        (not ref($continuation) or not $continuation->isa('PLIF::Exception::Internal::Continuation'))) {
        syntax 'Syntax error in continuation of "try" clause', caller;
    }
    eval { &$code };
    if (defined($continuation)) {
        if ($@ ne '') {
            if (ref($@) and $@->isa('PLIF::Exception')) {
                # yay, a standard exception
            } else {
                # an unexpected exception
                $@ = PLIF::Exception->create('message' => $@);
            }
        }
        return $continuation->handle($@);
    } else {
        return $@;
    }
}

sub handle($$) {
    my($class, $continuation, @more) = @_;
    syntax "Syntax error in \"handle ... with\" clause: \"$class\" is not a PLIF::Exception class", caller unless $class->isa('PLIF::Exception');
    if (not defined($continuation) or 
        not ref($continuation) or
        not $continuation->isa('PLIF::Exception::Internal::With')) {
        syntax 'Syntax error: missing "with" operator in "handle" clause', caller;
    }
    syntax 'Syntax error after "handle ... with" clause', caller if (scalar(@more));
    $continuation->{'resolved'} = 1;
    my $handler = $continuation->{'handler'};
    $continuation = $continuation->{'continuation'};
    if (not defined($continuation)) {
        $continuation = PLIF::Exception::Internal::Continuation->create(caller);
    }
    unshift(@{$continuation->{'handlers'}}, [$class, $handler]);
    return $continuation;
}

sub with(&;$) {
    my($handler, $continuation) = @_;
    if (not defined($continuation) or 
        not ref($continuation) or
        not $continuation->isa('PLIF::Exception::Internal::Continuation')) {
        syntax 'Syntax error after "handle ... with" clause', caller;
    }
    return PLIF::Exception::Internal::With->create($handler, $continuation, caller);
}

sub except(&;$) {
    my($handler, $continuation) = @_;
    if (defined($continuation) and
        (not ref($continuation) or
         not $continuation->isa('PLIF::Exception::Internal::Continuation') or
         defined($continuation->{'except'}) or
         scalar(@{$continuation->{'handlers'}}))) {
        syntax 'Syntax error after "except" clause', caller;
    }
    if (not defined($continuation)) {
        $continuation = PLIF::Exception::Internal::Continuation->create(caller);
    }
    $continuation->{'except'} = $handler;
    return $continuation;
}

sub finally(&;@) {
    my($handler, @continuation) = @_;
    syntax 'Missing semicolon after "finally" clause', caller if (scalar(@continuation));
    my $continuation = PLIF::Exception::Internal::Continuation->create(caller);
    $continuation->{'finally'} = $handler;
    return $continuation;
}

sub unhandled() {
    return PLIF::Exception::Internal::Unhandled->create(caller);
}


package PLIF::Exception::Internal::Continuation;

sub create {
    return bless {
        'handlers' => [],
        'except' => undef,
        'finally' => undef,
        'filename' => $_[2],
        'line' => $_[3],
        'resolved' => 0,
    }, $_[0];
}

sub handle {
    my $self = shift;
    my($exception) = @_;
    handler: while (1) {
        if (defined($exception)) {
            foreach my $handler (@{$self->{'handlers'}}) {
                if ($exception->isa($handler->[0])) {
                    my $result = &{$handler->[1]}($exception);
                    if (not defined($result) or
                        not ref($result) or
                        not $result->isa('PLIF::Exception::Internal::Unhandled')) {
                        last handler;
                    }
                    $result->{'resolved'} = 1;
                }
            }
            if (defined($self->{'except'})) {
                &{$self->{'except'}}($exception);
            }
        }
        last;
    }
    if (defined($self->{'finally'})) {
        &{$self->{'finally'}}();
    }
    $self->{'resolved'} = 1;
    return defined($exception);
}

sub DESTROY {
    my $self = shift;
    return $self->SUPER::DESTROY(@_) if $self->{'resolved'};
    my($package, $filename, $line) = caller;
    my $parts = 0x00;
    $parts |= 0x01 if scalar(@{$self->{'handlers'}});
    $parts |= 0x02 if defined($self->{'except'});
    $parts |= 0x04 if defined($self->{'finally'});
    my $messages = ["Incorrectly used PLIF::Exception::Internal::Continuation object at $self->{'filename'} line $self->{'line'}\n",
                    "Incorrectly used \"handle ... with\" clause at $self->{'filename'} line $self->{'line'}\n",
                    "Incorrectly used \"except\" clause at $self->{'filename'} line $self->{'line'}\n",
                    "Incorrectly used \"handle ... with\" and \"except\" clauses at $self->{'filename'} line $self->{'line'}\n",
                    "Incorrectly used \"finally\" clause at $self->{'filename'} line $self->{'line'}\n",
                    "Incorrectly used \"handle ... with\" and \"finally\" clauses at $self->{'filename'} line $self->{'line'}\n",
                    "Incorrectly used \"except\" and \"finally\" clauses at $self->{'filename'} line $self->{'line'}\n",
                    "Incorrectly used \"handle ... with\", \"except\", and \"finally\" clauses at $self->{'filename'} line $self->{'line'}\n",];
    warn $messages->[$parts]; # XXX can't raise an exception in a destructor
}


package PLIF::Exception::Internal::With;

sub create {
    return bless {
        'handler' => $_[1],
        'continuation' => $_[2],
        'filename' => $_[4],
        'line' => $_[5],
        'resolved' => 0,
    }, $_[0];
}

sub DESTROY {
    my $self = shift;
    return $self->SUPER::DESTROY(@_) if $self->{'resolved'};
    warn "Incorrectly used \"with\" operator at $self->{'filename'} line $self->{'line'}\n"; # XXX can't raise an exception in a destructor
}


package PLIF::Exception::Internal::Unhandled;

sub create {
    my($package, $filename, $line) = @_;
    return bless {
        'filename' => $_[2],
        'line' => $_[3],
        'resolved' => 0,
    }, $_[0];
}

sub DESTROY {
    my $self = shift;
    return $self->SUPER::DESTROY(@_) if $self->{'resolved'};
    warn "Incorrectly used \"unhandled\" function at $self->{'filename'} line $self->{'line'}\n"; # XXX can't raise an exception in a destructor
}
