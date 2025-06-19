namespace TrainerLib_Unloader
{
    internal static class Program
    {
        private static void Main(string[] args)
        {
            Unloader.CloseIfRunning(args.Length != 0 ? args[0] : null, true);
        }
    }
}
