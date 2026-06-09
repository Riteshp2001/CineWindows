/*
 * CineWindows Launcher
 * Compile with MSYS2 MINGW64:
 *   gcc -O2 -mwindows src/launcher.c -o CineWindows.exe
 *
 * This small launcher finds the MSYS2 MINGW64 installation,
 * sets up the environment, and launches CineWindows' Python entry point.
 */

#include <windows.h>
#include <shellapi.h>
#include <stdio.h>

static int
launch_cine (const char *cine_dir, const char *cmd_line)
{
  char python_path[MAX_PATH];
  char start_script[MAX_PATH];
  char gsettings_dir[MAX_PATH];
  char env_path[65536];
  char env_lang[] = "LANG=C";
  char env_lcall[] = "LC_ALL=C";
  char *msys2_dirs[] = {
    "C:\\msys64\\mingw64\\bin",
    "C:\\msys2\\mingw64\\bin",
    NULL
  };
  char *msys2_bin = NULL;
  int i;

  /* Find MSYS2 MINGW64 */
  for (i = 0; msys2_dirs[i] != NULL; i++)
    {
      snprintf (python_path, sizeof (python_path),
                "%s\\python.exe", msys2_dirs[i]);
      if (GetFileAttributesA (python_path) != INVALID_FILE_ATTRIBUTES)
        {
          msys2_bin = msys2_dirs[i];
          break;
        }
    }

  if (msys2_bin == NULL)
    {
      MessageBoxA (NULL,
                   "MSYS2 MINGW64 not found.\n"
                   "Please install MSYS2 to C:\\msys64 and run setup_windows.ps1.",
                   "CineWindows - Error",
                   MB_OK | MB_ICONERROR);
      return 1;
    }

  /* Build paths */
  snprintf (start_script, sizeof (start_script),
            "%s\\start_cine.py", cine_dir);
  snprintf (gsettings_dir, sizeof (gsettings_dir),
            "%s\\data", cine_dir);

  /* Set environment */
  snprintf (env_path, sizeof (env_path),
            "%s;%s", msys2_bin, getenv ("PATH") ? getenv ("PATH") : "");
  SetEnvironmentVariableA ("PATH", env_path);
  SetEnvironmentVariableA ("GSETTINGS_SCHEMA_DIR", gsettings_dir);
  SetEnvironmentVariableA ("LANG", "C");
  SetEnvironmentVariableA ("LC_ALL", "C");

  /* Launch */
  {
    SHELLEXECUTEINFOA shi = { 0 };
    shi.cbSize = sizeof (shi);
    shi.fMask = SEE_MASK_NOCLOSEPROCESS;
    shi.lpFile = python_path;
    char parameters[32768];
    snprintf (parameters, sizeof (parameters), "\"%s\" %s",
              start_script, cmd_line ? cmd_line : "");
    shi.lpParameters = parameters;
    shi.lpDirectory = cine_dir;
    shi.nShow = SW_SHOWNORMAL;

    if (!ShellExecuteExA (&shi))
      {
        MessageBoxA (NULL, "Failed to launch CineWindows.", "CineWindows - Error",
                     MB_OK | MB_ICONERROR);
        return 1;
      }

    WaitForSingleObject (shi.hProcess, INFINITE);
    CloseHandle (shi.hProcess);
  }

  return 0;
}

int WINAPI
WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance,
         LPSTR lpCmdLine, int nCmdShow)
{
  char cine_dir[MAX_PATH];
  char *p;
  int ret;

  /* Get the directory where CineWindows.exe lives */
  if (!GetModuleFileNameA (NULL, cine_dir, sizeof (cine_dir)))
    return 1;

  p = strrchr (cine_dir, '\\');
  if (p)
    *p = '\0';

  ret = launch_cine (cine_dir, lpCmdLine);

  return ret;
}
