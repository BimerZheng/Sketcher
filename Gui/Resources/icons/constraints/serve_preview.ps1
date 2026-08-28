$l = [System.Net.HttpListener]::new()
$l.Prefixes.Add("http://127.0.0.1:8899/")
$l.Start()
Write-Host "Serving preview on http://127.0.0.1:8899/"
while ($true) {
    $c = $l.GetContext()
    $p = $c.Request.Url.LocalPath
    if ($p -eq "/" -or $p -eq "") { $p = "/constraints/slope_preview.html" }
    $f = "D:\FreeCadDevelopment\FreeCAD\src\Mod\Sketcher\Gui\Resources\icons\" + $p.TrimStart("/")
    if (Test-Path $f) {
        $b = [IO.File]::ReadAllBytes($f)
        $ext = [IO.Path]::GetExtension($f).ToLower()
        $mime = switch ($ext) { ".html" {"text/html; charset=utf-8"} ".svg" {"image/svg+xml"} default {"application/octet-stream"} }
        $c.Response.ContentType = $mime
        $c.Response.OutputStream.Write($b, 0, $b.Length)
    } else {
        $c.Response.StatusCode = 404
    }
    $c.Response.Close()
}
