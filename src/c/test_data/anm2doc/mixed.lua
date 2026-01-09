--[==[PTK:{"version":1,"checksum":"0000000000000000","selectors":[{"group":"表情","items":[["通常","レイヤー/表情/通常"],["笑顔","レイヤー/表情/笑顔"]]},{"group":"目パチ","items":[{"script":"PSDToolKit.Blinker","n":"目パチアニメ","params":[["間隔(秒)","5.00"]]}]}],"psd":"C:/path/to/mixed.psd"}]==]
--label:混合テスト
--information:PSD Layer Selector for mixed.psd
--select@sel1:表情,通常=1,笑顔=2
--select@sel2:目パチ,目パチアニメ=1
require("PSDToolKit").psdcall(function()
require("PSDToolKit").add_layer_selector(1, function() return {
  "レイヤー/表情/通常",
  "レイヤー/表情/笑顔",
} end, sel1)
require("PSDToolKit").add_layer_selector(2, function() return {
  require("PSDToolKit.Blinker").new({
    ["間隔(秒)"] = "5.00",
  }),
} end, sel2)
end)
