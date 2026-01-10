--[==[PTK:{"version":1,"checksum":"0000000000000000","selectors":[{"group":"表情","items":[["通常","レイヤー/表情/通常"],["笑顔","レイヤー/表情/笑顔"]]}],"psd":"C:/path/to/test.psd"}]==]
--label:テストラベル
--information:PSD Layer Selector for test.psd
--select@sel1:表情,(None)=0,通常=1,笑顔=2
require("PSDToolKit").psdcall(function()
require("PSDToolKit").add_layer_selector(1, function() return {
  "レイヤー/表情/通常",
  "レイヤー/表情/笑顔",
} end, sel1)
end)
