import type { Metadata } from "next";
import "./globals.css";
import { TooltipProvider } from "@/components/ui/tooltip";
import Sidebar from "@/components/sidebar";

export const metadata: Metadata = {
  title: "CAN We Do It?",
  description: "Interface to interact with the CAN bus.",
};

export default function RootLayout({
  children,
}: Readonly<{
  children: React.ReactNode;
}>) {
  return (
    <html lang="en">
      <body>
        <TooltipProvider>
          <div className="grid h-screen w-full pl-[56px]">
            <Sidebar />
            <div className="flex flex-col">
              <header className="sticky top-0 z-10 flex h-[57px] items-center gap-1 border-b bg-background px-4">
                <h1 className="text-xl font-semibold">CAN We Do It?</h1>
              </header>
              <main className="grid flex-1 gap-4 p-4 relative">{children}</main>
            </div>
          </div>
        </TooltipProvider>
      </body>
    </html>
  );
}
