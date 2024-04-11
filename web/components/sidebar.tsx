"use client";
import React from "react";
import { Button } from "@/components/ui/button";
import {
  TooltipContent,
  Tooltip,
  TooltipTrigger,
} from "@/components/ui/tooltip";
import { LucideBrickWall, SquareTerminal, Triangle } from "lucide-react";
import { usePathname } from "next/navigation";
import Link from "next/link";

function Sidebar() {
  const pathname = usePathname();

  return (
    <aside className="inset-y fixed left-0 z-20 flex h-full flex-col border-r">
      <div className="border-b p-2">
        <Button variant="outline" size="icon" aria-label="Home">
          <Triangle className="size-5 fill-foreground" />
        </Button>
      </div>
      <nav className="grid gap-1 p-2">
        <Tooltip>
          <TooltipTrigger asChild>
            <Button
              variant="ghost"
              size="icon"
              className={`rounded-lg` + (pathname == "/" ? " bg-muted" : "")}
              asChild
            >
              <Link href="/">
                <SquareTerminal className="size-5" />
              </Link>
            </Button>
          </TooltipTrigger>
          <TooltipContent side="right" sideOffset={5}>
            Socket
          </TooltipContent>
        </Tooltip>
        <Tooltip>
          <TooltipTrigger asChild>
            <Button
              variant="ghost"
              size="icon"
              className={
                `rounded-lg` + (pathname === "/playground" ? " bg-muted" : "")
              }
              asChild
            >
              <Link href="/playground">
                <LucideBrickWall className="size-5" />
              </Link>
            </Button>
          </TooltipTrigger>
          <TooltipContent side="right" sideOffset={5}>
            Playground
          </TooltipContent>
        </Tooltip>
      </nav>
    </aside>
  );
}

export default Sidebar;
