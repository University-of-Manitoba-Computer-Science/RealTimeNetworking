"use client";

import { CornerDownLeft } from "lucide-react";

import { Button } from "@/components/ui/button";
import { Input } from "@/components/ui/input";
import { useState } from "react";
import { handleCommandSocket } from "@/actions/commandHandler";

const responses = [
  {
    time: 1712688624107,
    system: true,
    message:
      "Type a command to get started. Additional help is available by typing 'help'.",
  },
];

interface IMessage {
  time: number;
  system: boolean;
  message: string;
  error?: boolean;
}

export default function Dashboard() {
  const [messages, setMessages] = useState<IMessage[]>(responses);
  const [isWaiting, setIsWaiting] = useState<boolean>(false);

  async function sendMessage(message: string) {
    if (!message) return;

    setMessages((prevMessages) => [
      ...prevMessages,
      {
        time: Date.now().valueOf(),
        message,
        system: false,
      },
    ]);

    setIsWaiting(true);
    const res = await handleCommandSocket(message);
    if (res.success) {
      setMessages((prevMessages) => [
        ...prevMessages,
        {
          time: res.time,
          message: res.message,
          system: true,
        },
      ]);
    } else {
      setMessages((prevMessages) => [
        ...prevMessages,
        {
          time: res.time,
          message: res.message,
          system: true,
          error: true,
        },
      ]);
    }
    setIsWaiting(false);
  }

  return (
    <div className="absolute inset-0 flex flex-col rounded-xl p-4 lg:col-span-2">
      <div className="font-mono text-sm overflow-y-scroll flex-grow pb-8">
        {messages.map((response, index) =>
          response.system ? (
            <div
              key={index}
              className={`flex flex-col p-2 gap-1 border-b ${
                response.error ? "text-destructive" : ""
              }`}
            >
              <p>{response.message}</p>
            </div>
          ) : (
            <div key={index} className="flex flex-row p-2 gap-1">
              <span className="text-border">&gt;&nbsp;</span>
              <span>{response.message}</span>
            </div>
          )
        )}
        {isWaiting && (
          <div className="flex flex-row p-2 gap-1">
            <span className="animate-pulse">Waiting for response...</span>
          </div>
        )}
      </div>
      <form
        className="relative overflow-hidden rounded-lg border bg-background focus-within:ring-1 focus-within:ring-ring flex items-center flex-shrink-0"
        onSubmit={(event) => {
          event.preventDefault();
          const command = event.currentTarget.elements.namedItem(
            "command"
          ) as HTMLInputElement;
          sendMessage(command.value);
          command.value = "";
        }}
      >
        <Input
          id="command"
          placeholder="Type your command here..."
          className="border-0 p-3 shadow-none focus-visible:ring-0 font-mono"
        />
        <Button type="submit" size="sm" className="m-2 ml-auto gap-1.5">
          Send Message
          <CornerDownLeft className="size-3.5" />
        </Button>
      </form>
    </div>
  );
}
