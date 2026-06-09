/*
 * CineWindows Launcher
 * Compile with MSYS2 MINGW64:
 *   gcc -O2 -mwindows src/launcher.c src/launcher.res -o CineWindows.exe
 *
 * Launches CineWindows using bundled Python runtime.
 * If no bundled runtime is found, falls back to MSYS2.
 */

#include <windows.h>
#include <shellapi.h>
#include <stdio.h>

static int
launch_cine (const char *cine_dir, const char *cmd_line)
{
  char python_path[MAX_PATH];
  char start_script[MAX_PATH];
  char runtime_bin[MAX_PATH];
  char gsettings_dir[MAX_PATH];
  char env_path[65536];
  char env_lang[] = "LANG=C";
  char env_lcall[] = "LC_ALL=C";
  int use_bundled = 0;

  /* 1. Try bundled Python runtime first (use pythonw.exe for no terminal) */
  snprintf (runtime_bin, sizeof (runtime_bin),
            "%s\\runtime\\bin\\pythonw.exe", cine_dir);
  if (GetFileAttributesA (runtime_bin) != INVALID_FILE_ATTRIBUTES)
    {
      snprintf (python_path, sizeof (python_path), "%s", runtime_bin);
      use_bundled = 1;
    }
  else
    {
      /* Try bundled python.exe fallback */
      snprintf (runtime_bin, sizeof (runtime_bin),
                "%s\\runtime\\bin\\python.exe", cine_dir);
      if (GetFileAttributesA (runtime_bin) != INVALID_FILE_ATTRIBUTES)
        {
          snprintf (python_path, sizeof (python_path), "%s", runtime_bin);
          use_bundled = 1;
        }
    }

  if (!use_bundled)
    {
      /* 2. Fall back to MSYS2 MINGW64 */
      char *msys2_dirs[] = {
        "C:\\msys64\\mingw64\\bin",
        "C:\\msys2\\mingw64\\bin",
        NULL
      };
      char *msys2_bin = NULL;

      for (int i = 0; msys2_dirs[i] != NULL; i++)
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
                       "CineWindows requires its bundled runtime.\n"
                       "Please reinstall CineWindows using the official installer.",
                       "CineWindows - Error",
                       MB_OK | MB_ICONERROR);
          return 1;
        }

      /* Set PATH to MSYS2 bin for legacy fallback */
      snprintf (env_path, sizeof (env_path),
                "%s;%s", msys2_bin, getenv ("PATH") ? getenv ("PATH") : "");
    }

  /* Build paths */
  snprintf (start_script, sizeof (start_script),
            "%s\\start_cine.py", cine_dir);
  snprintf (gsettings_dir, sizeof (gsettings_dir),
            "%s\\data", cine_dir);

  /* Set environment */
  if (use_bundled)
    {
      char bundle_root[MAX_PATH];
      char bundle_bin[MAX_PATH];
      snprintf (bundle_root, sizeof (bundle_root), "%s\\runtime", cine_dir);
      snprintf (bundle_bin, sizeof (bundle_bin), "%s\\runtime\\bin", cine_dir);
      snprintf (env_path, sizeof (env_path),
                "%s;%s\\runtime\\lib\\gio\\modules;%s",
                bundle_bin, cine_dir,
                getenv ("PATH") ? getenv ("PATH") : "");
      SetEnvironmentVariableA ("PYTHONHOME", bundle_root);
    }

  SetEnvironmentVariableA ("PATH", env_path);
  SetEnvironmentVariableA ("GSETTINGS_SCHEMA_DIR", gsettings_dir);
  SetEnvironmentVariableA ("LANG", "C");
  SetEnvironmentVariableA ("LC_ALL", "C");

  /* Ensure src/ is importable */
  {
    char pythonpath[MAX_PATH];
    snprintf (pythonpath, sizeof (pythonpath), "%s\\src", cine_dir);
    SetEnvironmentVariableA ("PYTHONPATH", pythonpath);
  }

  /* Set GI_TYPELIB_PATH for GObject introspection */
  {
    char typelib_path[MAX_PATH];
    snprintf (typelib_path, sizeof (typelib_path),
              "%s\\runtime\\lib\\girepository-1.0", cine_dir);
    SetEnvironmentVariableA ("GI_TYPELIB_PATH", typelib_path);
  }

  /* Set XDG_DATA_DIRS for icon theme */
  {
    char xdg_dir[MAX_PATH];
    snprintf (xdg_dir, sizeof (xdg_dir),
              "%s\\runtime\\share", cine_dir);
    SetEnvironmentVariableA ("XDG_DATA_DIRS", xdg_dir);
  }

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
