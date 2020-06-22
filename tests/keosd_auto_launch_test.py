#!/usr/bin/env python3

# This script tests that remcli launches remvault automatically when remvault is not
# running yet.

import subprocess


def run_remcli_wallet_command(command: str, no_auto_remvault: bool):
    """Run the given remcli command and return subprocess.CompletedProcess."""
    args = ['./programs/remcli/remcli']

    if no_auto_remvault:
        args.append('--no-auto-remvault')

    args += 'wallet', command

    return subprocess.run(args,
                          check=False,
                          stdout=subprocess.DEVNULL,
                          stderr=subprocess.PIPE)


def stop_remvault():
    """Stop the default remvault instance."""
    run_remcli_wallet_command('stop', no_auto_remvault=True)


def check_remcli_stderr(stderr: bytes, expected_match: bytes):
    if expected_match not in stderr:
        raise RuntimeError("'{}' not found in {}'".format(
            expected_match.decode(), stderr.decode()))


def remvault_auto_launch_test():
    """Test that keos auto-launching works but can be optionally inhibited."""
    stop_remvault()

    # Make sure that when '--no-auto-remvault' is given, remvault is not started by
    # remcli.
    completed_process = run_remcli_wallet_command('list', no_auto_remvault=True)
    assert completed_process.returncode != 0
    check_remcli_stderr(completed_process.stderr, b'Failed to connect to remvault')

    # Verify that remvault auto-launching works.
    completed_process = run_remcli_wallet_command('list', no_auto_remvault=False)
    if completed_process.returncode != 0:
        raise RuntimeError("Expected that remvault would be started, "
                           "but got an error instead: {}".format(
                               completed_process.stderr.decode()))
    check_remcli_stderr(completed_process.stderr, b'launched')


try:
    remvault_auto_launch_test()
finally:
    stop_remvault()
