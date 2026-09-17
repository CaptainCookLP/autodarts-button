chrome.commands.onCommand.addListener(command=>{if(command==='red-button')chrome.tabs.query({active:true,currentWindow:true},tabs=>{if(tabs[0]?.id)chrome.tabs.reload(tabs[0].id)})});

