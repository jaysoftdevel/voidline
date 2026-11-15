"""Resizable editor dialogs: readable fonts and workspace-sized status reports."""
import math
import tkinter as tk
from tkinter import ttk, font as tkfont

DIALOG_FONT = ("DejaVu Sans", 12)
DIALOG_TITLE_FONT = ("DejaVu Sans", 13, "bold")

def dialog_dimensions(message, screen_width, screen_height):
    """Screen-bounded initial size, with room for text and sensible wrapping."""
    lines = str(message).splitlines() or [""]
    width = min(max(420, min(900, max(len(line) for line in lines) * 7 + 90)),
                max(280, screen_width-60))
    chars_per_line=max(25,(width-72)//8)
    rows=sum(max(1,math.ceil(len(line)/chars_per_line)) for line in lines)
    height=min(max(170, rows*22+140), max(170,screen_height-100))
    return int(width),int(height)

class StudioDialog(tk.Toplevel):
    def __init__(self, parent, title, message, buttons=("OK",), prompt=None, minvalue=None, maxvalue=None):
        super().__init__(parent)
        self.title(title)
        self.transient(parent)
        self.resizable(True,True)
        self.result=None
        self.prompt=prompt
        self.minvalue,self.maxvalue=minvalue,maxvalue
        width,height=dialog_dimensions(message,self.winfo_screenwidth(),self.winfo_screenheight())
        if prompt is not None:height=max(height,230)
        self.minsize(min(350,width),min(150,height))
        self.geometry(f"{width}x{height}")
        self.columnconfigure(0,weight=1)
        self.rowconfigure(1,weight=1)
        ttk.Label(self,text=title,font=DIALOG_TITLE_FONT,padding=(16,12,16,5)).grid(row=0,column=0,sticky="ew")
        body=ttk.Frame(self,padding=(16,0,16,8))
        body.grid(row=1,column=0,sticky="nsew")
        body.columnconfigure(0,weight=1)
        body.rowconfigure(0,weight=1)
        # Read-only text supports arbitrarily long status reports and resize.
        self.detail=tk.Text(body,wrap="word",font=DIALOG_FONT,relief="flat",
                            background=self.cget("background"),borderwidth=0,highlightthickness=0,
                            padx=4,pady=7)
        self.detail.insert("1.0",str(message));self.detail.configure(state="disabled")
        self.detail.grid(row=0,column=0,sticky="nsew")
        scrollbar=ttk.Scrollbar(body,orient="vertical",command=self.detail.yview)
        self.detail.configure(yscrollcommand=scrollbar.set)
        scrollbar.grid(row=0,column=1,sticky="ns")
        self.value=tk.StringVar()
        self.error=tk.StringVar()
        if prompt is not None:
            self.input=ttk.Entry(body,textvariable=self.value,font=DIALOG_FONT)
            self.input.grid(row=1,column=0,columnspan=2,sticky="ew",pady=(7,2))
            ttk.Label(body,textvariable=self.error,foreground="#ad3d37",font=DIALOG_FONT).grid(
                row=2,column=0,columnspan=2,sticky="w")
        controls=ttk.Frame(self,padding=(12,5,12,12))
        controls.grid(row=2,column=0,sticky="e")
        for option in buttons:
            ttk.Button(controls,text=option,width=max(8,len(option)+2),
                       command=lambda choice=option:self.select(choice)).pack(side="left",padx=4)
        self.bind("<Escape>",lambda _e:self.select("Cancel"))
        self.bind("<Return>",lambda _e:self.select(buttons[0]))
        self.protocol("WM_DELETE_WINDOW",lambda:self.select("Cancel"))
        self.update_idletasks()
        x=max(0,parent.winfo_rootx()+(parent.winfo_width()-width)//2)
        y=max(0,parent.winfo_rooty()+(parent.winfo_height()-height)//2)
        self.geometry(f"+{min(x,self.winfo_screenwidth()-width)}+{min(y,self.winfo_screenheight()-height)}")
        self.wait_visibility()
        self.grab_set()
        if prompt is not None:self.input.focus_set()
        else:self.focus_set()

    def select(self,choice):
        if self.prompt is not None and choice not in ("Cancel","No"):
            value=self.value.get().strip()
            if self.prompt=="integer":
                try:parsed=int(value)
                except ValueError:
                    self.error.set("Enter a whole number.");return
                if (self.minvalue is not None and parsed<self.minvalue) or (self.maxvalue is not None and parsed>self.maxvalue):
                    self.error.set(f"Enter a number from {self.minvalue} to {self.maxvalue}.");return
                self.result=parsed
            else:self.result=value
        else:self.result=choice
        self.grab_release()
        self.destroy()

    def show(self):
        self.wait_window()
        return self.result
