using System;
using System.Linq;
using System.Windows.Forms;

namespace Udm {
    public sealed class SiteLoginDialog : Form {
        public SiteLoginDialog(Manager manager,SiteLogin existing){
            ClassicLayout.Form(this,"Site login",300,160);Tag="light";
            ClassicLayout.Label(this,"Site (HTTPS)",7,12,85);var site=ClassicLayout.Text(this,existing==null?"https://":existing.Origin,96,9,197,existing!=null);
            ClassicLayout.Label(this,"User name",7,39,85);var user=ClassicLayout.Text(this,existing==null?"":existing.UserName,96,36,197);
            ClassicLayout.Label(this,"Password",7,66,85);var secret=ClassicLayout.Text(this,existing==null?"":Secrets.Reveal(existing.ProtectedPassword),96,63,197);secret.UseSystemPasswordChar=true;
            ClassicLayout.Label(this,"Used for HTTP Basic authentication on this exact site and port. Passwords are encrypted for your Windows account.",7,92,286,32);
            AcceptButton=ClassicLayout.Button(this,"OK",177,136,55,()=>{manager.SaveSiteLogin(site.Text,user.Text,secret.Text);DialogResult=DialogResult.OK;Close();});CancelButton=ClassicLayout.Button(this,"Cancel",238,136,55,Close);
        }
    }
    public sealed class CategoryRuleDialog : Form {
        public CategoryRuleDialog(Manager manager,string category){
            ClassicLayout.Form(this,"Category properties",316,180);Tag="light";
            var rule=manager.State.Settings.CategoryRules.FirstOrDefault(r=>r.Category==category);
            ClassicLayout.Label(this,ClassicLayout.CategoryLabel(category),7,9,302);
            ClassicLayout.Label(this,"File types (separated by spaces)",7,32,302);var extensions=ClassicLayout.Text(this,rule==null?"":rule.Extensions,7,49,302);
            ClassicLayout.Label(this,"Only on these sites (blank means any site)",7,78,302);var hosts=ClassicLayout.Text(this,rule==null?"":rule.Hosts,7,95,302);
            ClassicLayout.Label(this,"Example: zip pdf — use * for any extension. Leave file types empty to remove the custom rule.",7,119,302,25);
            AcceptButton=ClassicLayout.Button(this,"OK",193,156,55,()=>{manager.SaveCategoryRule(category,extensions.Text,hosts.Text);DialogResult=DialogResult.OK;Close();});CancelButton=ClassicLayout.Button(this,"Cancel",254,156,55,Close);
        }
    }
}
