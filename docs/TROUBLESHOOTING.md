## Troubleshooting

**Cannot find running process 'fm.exe'**

- Start Football Manager 24 and load the target save before starting the application.

**Memory access is denied**

- Make sure the application has permission to inspect and modify the Football Manager process. Linux systems may
  restrict `/proc/<pid>/mem` or ptrace access, while Windows may require the appropriate process rights.
- On Linux, make sure the `pidof` command is available.