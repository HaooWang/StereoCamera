
filename='�Ӳ�����.txt';
data = importdata(filename);
r = data(1);    % ����
c = data(2);    % ����
disp = data(3:end); % �Ӳ�
vmin = min(disp);
vmax = max(disp);
disp = reshape(disp, [c,r])'; % ����������ʽ�� disp �ع�Ϊ ������ʽ
%  OpenCV ����ɨ��洢ͼ��Matlab ����ɨ��洢ͼ��
%  �ʶ� disp ���������������ȱ�� c �� r �еľ���Ȼ����ת�û� r �� c ��
img = uint8( 255 * ( disp - vmin ) / ( vmax - vmin ) );
mesh(disp);
set(gca,'YDir','reverse');  % ͨ�� mesh ��ʽ��ͼʱ���赹�� Y �᷽��
axis tight; % ʹ��������ʾ��Χ�����ݷ�Χ�����ϣ�ȥ���հ���ʾ��
 