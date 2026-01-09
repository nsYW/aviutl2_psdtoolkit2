--[==[PTK:{"version":1,"checksum":"0000000000000000","selectors":[{"group":"目パチ","items":[{"script":"PSDToolKit.Blinker","n":"目パチアニメ","params":[["間隔(秒)","5.00"],["開き時間(秒)","0.06"]]}]}],"psd":"C:/path/to/anim.psd"}]==]
--label:アニメーションテスト
--information:PSD Layer Selector for anim.psd
--select@sel1:目パチ,目パチアニメ=1
require("PSDToolKit").psdcall(function()
require("PSDToolKit").add_layer_selector(1, function() return {
  require("PSDToolKit.Blinker").new({
    ["間隔(秒)"] = "5.00",
    ["開き時間(秒)"] = "0.06",
  }),
} end, sel1)
end)
