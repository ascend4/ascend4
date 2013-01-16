var useConnectors = true;
var connectors;
var connections = new Array();
var detail = 100;

$(function()
{
	//Initialise jsPlumb for the connections between elements.
	jsPlumb.Defaults.PaintStyle = {
		lineWidth:1,
		strokeStyle: 'rgba(0,0,0,100)',
	}
});

function handleEnterKeypress()
{
	if(event.keyCode == 13)
	{
		changeSettings();
		changeWidth();
	}
	return event.keyCode!=13;
}

function changeSettings()
{
	//CONNECTORS:
	var willUseConnectors = document.getElementById('lines').checked;
	var lessElementSpace = document.getElementById('lessElementSpace');
	if(useConnectors && !willUseConnectors)
	{
		if(!lessElementSpace)
		{
			var elementSheet = document.createElement('style');
			elementSheet.id = 'lessElementSpace';
			elementSheet.innerHTML = ".branch {margin-top: 2px;}\n.elementContainer {margin-bottom: 2px;}\n.modifierContainer {margin-bottom: 2px;}";
			document.body.appendChild(elementSheet);
		}
		// Remove all connectors
		removeConnectors();
	}
	else if(!useConnectors && willUseConnectors)
	{
		if(lessElementSpace)
		{
			lessElementSpace.parentNode.removeChild(lessElementSpace);
		}
		// Draw all connectors
		drawConnectors();
	}
	useConnectors = willUseConnectors;
	
	//BOXES:
	var useBoxes = document.getElementById('boxes').checked;
	
	var node = document.getElementById('boxStyle');
	if(node)
	{
		node.parentNode.removeChild(node);
	}
	var sheet = document.createElement('style');
	sheet.id = 'boxStyle';
	if(useBoxes)
	{
		sheet.innerHTML = ".branch {border: 1px solid #999;}";
	}
	else
	{
		sheet.innerHTML = "";
	}
	document.body.appendChild(sheet);
}

function changeWidth()
{
	var newWidth = document.getElementById('width').value;
	
	if(useConnectors)
	{
		// Remove all connectors
		removeConnectors();
	}
	
	document.body.style.width = newWidth + "%";
	
	if(useConnectors)
	{
		// Draw all connectors
		drawConnectors();
	}
	
}

function changeDetail()
{
	var newDetail = document.getElementById('detail').value;
	
	if(!compareDetail(newDetail, detail))
	{
		if(useConnectors)
		{
			// Remove all connections
			removeConnectors();
		}
		if(newDetail < 50)
		{
			var node = document.getElementById('annotationStyle');
			if(!node)
			{	
				var sheet = document.createElement('style');
				sheet.innerHTML = ".annotation {display: none;}";
				sheet.id = 'annotationStyle';
				document.body.appendChild(sheet);
			}
		}
		else if(newDetail < 100)
		{
			var node = document.getElementById('annotationStyle');
			if(node)
			{
				node.parentNode.removeChild(node);
			}
		}
		if(useConnectors)
		{
			// Show all connectors in their new positions
			connectors = drawConnectors();
		}
		detail = newDetail;
	}
}

function compareDetail(left, right)
{	
	var bounds = [0, 50, 100];
	for(var i = 0; i < bounds.length-1; i++)
	{
		if(left > bounds[i] && left < bounds[i+1]
			&& right > bounds[i] && right < bounds[i+1])
		{
			return true;
		}
	}
	return false;
}

// TODO: The connections code seems to be massively inefficient.
// At the moment we detach every connection, an then redraw them all from scratch.
// Even this is not done efficiently I suspect.
function drawConnectors()
{
	var connectors = new Array();

	for(var i=0; i<connections.length; i++)
	{
		if(sourceAndTargetAreVisible(connections[i]))
		{
			connectors.push(
				jsPlumb.connect({
				source:connections[i].source,
				target:connections[i].target,
				anchors:["BottomCenter", "TopCenter" ],
				connector:['Bezier', {curviness: 40}],
				endpoint:'Blank',
			}));
		}
	}
	
	return connectors;
}

function removeConnectors()
{
	if(connectors)
	{
		for(var i=connectors.length-1; i>=0; i--)
		{
			jsPlumb.select(connectors[i].connector).detach();
		}
	}
}

function showChildren(self)
{
	var firstContainingElement = self.parentNode;
	var secondContainingElement = firstContainingElement.parentNode;
	var containingDiv = secondContainingElement.parentNode;
	
	//Show all immediate children
	if (containingDiv.hasChildNodes())
	{
		if(useConnectors)
		{
			// Remove all connections
			removeConnectors();
		}
		// Get all children of node
		var children = containingDiv.childNodes;               
		
		// Loop through the children
		for(var c=0; c < children.length; c++)
		{
			if(children[c].className == 'branch')
			{
				children[c].style.display = 'inherit';
				connections.push({source:secondContainingElement, target:children[c]});
			}
		}
		if(useConnectors)
		{
			// Show all connectors in their new positions
			connectors = drawConnectors();
		}
	}
	
	//Toggle to minus sign
	self.style.display = 'none';
	
	var images = firstContainingElement.childNodes;
	for(var i=0; i<images.length; i++)
	{
		if(images[i].className == 'hide')
		{
			images[i].style.display = 'inherit';
		}
	}
}

function hideChildren(self)
{
	var firstContainingElement = self.parentNode;
	var secondContainingElement = firstContainingElement.parentNode;
	var containingDiv = secondContainingElement.parentNode;
	
	//Show all immediate children
	if (containingDiv.hasChildNodes())
	{
		if(useConnectors)
		{
			// Remove all connections
			removeConnectors();
		}
		
		// Get all children of node
		var children = containingDiv.childNodes;               
		
		// Loop through the children
		for(var c=0; c < children.length; c++)
		{
			if(children[c].className == 'branch')
			{
				children[c].style.display = 'none';
				//TODO: Fix this logic
				var index = connections.indexOf({source:secondContainingElement, target:children[c]});
				if(index > -1) {connections.splice(index, 1);}
				//END TODO.
			}
		}
		
		if(useConnectors)
		{
			// Show all connectors in their new positions
			connectors = drawConnectors();
		}
	}
	
	//Toggle to minus sign
	self.style.display = 'none';
	
	var images = firstContainingElement.childNodes;
	for(var i=0; i<images.length; i++)
	{
		if(images[i].className == 'show')
		{
			images[i].style.display = 'inherit';
		}
	}
}

function sourceAndTargetAreVisible(connection)
{
	return connection.source.style.display != 'none'
			&& connection.target.style.display != 'none';
}