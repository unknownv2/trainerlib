using System.IO;
using System.Threading;

namespace TrainerLib_Unloader
{
    internal class Unloader
    {
        public static bool IsFileInUse(string filename)
        {
            if (!File.Exists(filename))
            {
                return false;
            }

            try
            {
                // This will fail if the DLL is loaded into a game.
                using (new FileStream(filename, FileMode.Open, FileAccess.ReadWrite, FileShare.Read))
                {
                    return false;
                }
            }
            catch
            {
                return true;
            }
        }

        public static void CloseIfRunning(string dllPath = null, bool deleteDll = false)
        {
            string destFileName = null;

            EventWaitHandle handle;

            if (dllPath == null)
            {
                deleteDll = false;
            }
            else
            {
                // If the DLL doesn't exist, it must not be running.
                if (!File.Exists(dllPath))
                {
                    return;
                }

                // If we don't have to delete the DLL, make a copy of it
                // cause we have to delete it anyway for testing.
                if (!deleteDll)
                {
                    destFileName = Path.GetTempFileName();
                    File.Copy(dllPath, destFileName, true);
                }
                try
                {
                    File.Delete(dllPath);
                    if (!deleteDll)
                    {
                        File.Move(destFileName, dllPath);
                    }
                    return;
                }
                catch
                {
                    //
                }
            }

            // Try to open the event. If it doesn't exist, the DLL isn't running.
            if (!EventWaitHandle.TryOpenExisting(@"Local\Infinity_Trainer", out handle))
            {
                return;
            }

            var result = !handle.Set() || (dllPath == null);

            handle.Close();

            if (result)
            { 
                return;
            }

            var flag = false;

            for (var i = 0; i < 10; i++)
            {
                try
                {
                    File.Delete(dllPath);
                    flag = true;
                    break;
                }
                catch
                {
                    Thread.Sleep(500);
                }
            }

            if (flag && !deleteDll)
            {
                File.Move(destFileName, dllPath);
            }
        }
    }
}
